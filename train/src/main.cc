#include "chess.hh"
#include "eval.hh"
#include "process.hh"
#include "torch/torch.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <limits>
#include <random>
#include <rocksdb/db.h>
#include <span>
#include <sstream>
#include <string>
#include <vector>

using namespace chess;
using namespace segfault;

static constexpr auto BOARD_SIZE_NNUE = 768;

std::array<float, BOARD_SIZE_NNUE>
encode_board(const Board & board) {
    std::array<float, BOARD_SIZE_NNUE> input{};

    auto indices = board.occ();
    while (!indices.empty()) {
        const auto sq = indices.msb();
        const auto piece = board.at(sq);
        const auto piece_index = static_cast<int>(piece);
        input[piece_index * 64 + static_cast<int>(sq)] = 1.0f;
        indices.clear(sq);
    }

    return input;
}

static void
save_module(const torch::nn::Module & m, const std::string & path) {
    torch::serialize::OutputArchive archive;
    m.save(archive);
    archive.save_to(path);
}

struct NetImpl : torch::nn::Module {
    torch::nn::Sequential seq;

    explicit NetImpl(int64_t input_dim)
        : seq(torch::nn::Sequential(torch::nn::Linear(input_dim, 512), torch::nn::ReLU(),
                                    torch::nn::Linear(512, 512), torch::nn::ReLU(),
                                    torch::nn::Linear(512, 256), torch::nn::ReLU(),
                                    torch::nn::Linear(256, 1))) {
        register_module("seq", seq);
    }

    torch::Tensor
    forward(torch::Tensor x) {
        return seq->forward(x);
    }
};

TORCH_MODULE(Net);

static void
load_module(torch::nn::Module & m, const std::string & path) {
    torch::serialize::InputArchive archive;
    archive.load_from(path);
    m.load(archive);
}

static auto
build_keys(rocksdb::DB * const database, std::size_t max_samples) {
    auto *                   it = database->NewIterator(rocksdb::ReadOptions());
    std::vector<PackedBoard> keys;

    for (it->SeekToFirst(); it->Valid(); it->Next()) {
        if (keys.size() >= max_samples) {
            break;
        }

        auto compact = Board::Compact::encode(it->key().ToString());
        keys.push_back(std::move(compact));
    }

    delete it;
    return keys;
}

struct FenEvalDataset : torch::data::datasets::Dataset<FenEvalDataset> {
    rocksdb::DB *          database_;
    std::span<PackedBoard> keys_;

    FenEvalDataset(rocksdb::DB * const database, const std::span<PackedBoard> & keys)
        : database_(database), keys_(keys) {}

    torch::data::Example<>
    get(std::size_t index) override {
        std::string value;
        const auto  board = Board::Compact::decode(keys_[index]);
        database_->Get(rocksdb::ReadOptions(), board.getFen(), &value);

        int cp = 0;
        try {
            cp = std::stoi(value);
        } catch (...) {
            cp = 0;
        }

        constexpr auto k = 0.00368208f;
        const auto     score = 1.0f / (1.0f + std::exp(-k * cp));
        const auto     enc = encode_board(board); // std::array<float, BOARD_SIZE_NNUE>

        auto x = torch::from_blob((void *)enc.data(), {static_cast<int64_t>(BOARD_SIZE_NNUE)},
                                  torch::TensorOptions().dtype(torch::kFloat32))
                     .clone();

        auto y = torch::tensor({score}, torch::TensorOptions().dtype(torch::kFloat32));
        return {x, y};
    }

    torch::optional<size_t>
    size() const override {
        return keys_.size();
    }
};

int
main() {
    torch::manual_seed(1);

    torch::Device  device{torch::kCUDA};
    constexpr auto max_samples = 1'200'000'000;

    const std::string new_data_path = "./fens-32m-norm"; // <-- your new DB
    const std::string base_model_path = "model_best.pt";

    rocksdb::DB *    database;
    rocksdb::Options options;
    rocksdb::DB::Open(options, new_data_path, &database);

    auto keys = build_keys(database, max_samples);
    if (keys.empty()) {
        throw std::runtime_error("No samples indexed.");
    }
    std::cout << "Indexed samples: " << keys.size() << std::endl;

    std::mt19937_64 rng{1};
    std::shuffle(keys.begin(), keys.end(), rng);

    const std::size_t            val_n = std::min<std::size_t>(200'000, keys.size() / 20);
    const std::span<PackedBoard> val_keys{keys.begin(), keys.begin() + val_n};
    const std::span<PackedBoard> train_keys{keys.begin() + val_n, keys.end()};

    auto train_ds = FenEvalDataset(database, train_keys).map(torch::data::transforms::Stack<>());
    auto val_ds = FenEvalDataset(database, val_keys).map(torch::data::transforms::Stack<>());

    const int64_t batch_size = 1024;
    const int     epochs = 64;

    auto train_loader = torch::data::make_data_loader(
        std::move(train_ds),
        torch::data::DataLoaderOptions().batch_size(batch_size).workers(4).drop_last(true));
    auto val_loader = torch::data::make_data_loader(
        std::move(val_ds),
        torch::data::DataLoaderOptions().batch_size(batch_size).workers(2).drop_last(false));

    // Construct, load pretrained weights, THEN move to device.
    Net model{BOARD_SIZE_NNUE};
    load_module(*model, base_model_path);
    model->to(device);
    std::cout << "Loaded base weights from " << base_model_path << std::endl;

    // Smaller LR + weight decay; optimizer state restarts (we don't persist it).
    torch::optim::AdamW optimizer{model->parameters(),
                                  torch::optim::AdamWOptions(1e-4).weight_decay(1e-4)};

    const double lr_max = 1e-4;
    const double lr_min = 1e-6;

    auto evaluate = [&]() {
        model->eval();
        double             sum_sq = 0.0;
        int64_t            count = 0;
        torch::NoGradGuard guard;
        for (auto & batch : *val_loader) {
            const auto xb = batch.data.to(device, true);
            const auto yb = batch.target.to(device, true).view({-1, 1});
            const auto pred = model->forward(xb);
            sum_sq += torch::mse_loss(pred, yb, torch::Reduction::Sum).item().to<double>();
            count += yb.size(0);
        }
        return (count > 0) ? sum_sq / static_cast<double>(count) : 0.0;
    };

    // Baseline before any fine-tuning — tells you how the current model already does
    // on the new SF18 distribution. If this is already very low, fine-tuning may not help much.
    std::cout << "epoch 0 (baseline) | val mse: " << evaluate() << "\n";

    float best_loss = std::numeric_limits<float>::infinity();

    for (int epoch = 1; epoch <= epochs; epoch++) {
        const double learning_rate =
            lr_min + 0.5 * (lr_max - lr_min) * (1.0 + std::cos(M_PI * epoch / epochs));
        for (auto & group : optimizer.param_groups()) {
            static_cast<torch::optim::AdamWOptions &>(group.options()).lr(learning_rate);
        }

        model->train();
        for (auto & batch : *train_loader) {
            const auto xb = batch.data.to(device, true);
            const auto yb = batch.target.to(device, true).view({-1, 1});

            optimizer.zero_grad();
            const auto pred = model->forward(xb);
            auto       loss = torch::mse_loss(pred, yb, torch::Reduction::Mean);
            loss.backward();
            optimizer.step();
        }

        const double val_mse = evaluate();
        std::cout << "epoch " << epoch << " | lr " << learning_rate << " | val mse: " << val_mse
                  << "\n";

        std::ostringstream name;
        name << "finetune_epoch_" << std::setw(3) << std::setfill('0') << epoch << ".pt";
        save_module(*model, name.str());

        if (val_mse < best_loss) {
            best_loss = val_mse;
            save_module(*model, "finetune_best.pt");
        }
    }

    save_module(*model, "finetune_final.pt");
    return 0;
}
