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

static void
load_module(torch::nn::Module & m, const std::string & path) {
    torch::serialize::InputArchive archive;
    archive.load_from(path);
    m.load(archive);
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

struct Sample {
    PackedBoard board;
    float       target;
};

static auto
build_samples(rocksdb::DB * const database, const std::size_t max_samples) {
    constexpr auto k = 0.00368208f;
    auto * const   it = database->NewIterator(rocksdb::ReadOptions());

    std::vector<Sample> samples;
    samples.reserve(max_samples);

    for (it->SeekToFirst(); it->Valid(); it->Next()) {
        if (samples.size() >= max_samples) {
            break;
        }

        int cp{0};
        try {
            cp = std::stoi(it->value().ToString());
        } catch (...) {
            continue;
        }

        const auto target = 1.0f / (1.0f + std::exp(-k * cp));
        samples.push_back(Sample{Board::Compact::encode(it->key().ToString()), target});
    }

    delete it;
    return samples;
}

struct FenEvalDataset : torch::data::datasets::Dataset<FenEvalDataset> {
    std::span<const Sample> samples_;

    explicit FenEvalDataset(const std::span<const Sample> & samples) : samples_(samples) {}

    torch::data::Example<>
    get(std::size_t index) override {
        const auto & sample = samples_[index];
        const auto   board = Board::Compact::decode(sample.board);
        const auto   enc = encode_board(board);

        auto x = torch::from_blob((void *)enc.data(), {static_cast<int64_t>(BOARD_SIZE_NNUE)},
                                  torch::TensorOptions().dtype(torch::kFloat32))
                     .clone();
        auto y = torch::tensor({sample.target}, torch::TensorOptions().dtype(torch::kFloat32));
        return {x, y};
    }

    torch::optional<size_t>
    size() const override {
        return samples_.size();
    }
};

int
main() {
    torch::manual_seed(1);

    torch::Device     device{torch::kCUDA};
    constexpr auto    max_samples = 1'200'000'000;
    const std::string path = "./fens-1.2b-norm";
    rocksdb::DB *     database{nullptr};
    rocksdb::Options  options;

    const auto status = rocksdb::DB::Open(options, path, &database);
    if (!status.ok()) {
        throw std::runtime_error{"DB open failed: " + status.ToString()};
    }

    auto samples = build_samples(database, max_samples);
    if (samples.empty()) {
        throw std::runtime_error{"No samples indexed."};
    }
    std::cout << "Indexed samples: " << samples.size() << std::endl;

    std::mt19937_64 rng{1};
    std::shuffle(samples.begin(), samples.end(), rng);

    const std::size_t             val_n = std::min<std::size_t>(1'000'000, samples.size() / 20);
    const std::span<const Sample> val_samples{samples.begin(), samples.begin() + val_n};
    const std::span<const Sample> train_samples{samples.begin() + val_n, samples.end()};
    std::cout << "Train/val split done." << std::endl;

    auto train_ds = FenEvalDataset(train_samples).map(torch::data::transforms::Stack<>());
    auto val_ds = FenEvalDataset(val_samples).map(torch::data::transforms::Stack<>());

    const int64_t batch_size = 4096;
    const int     epochs = 64;
    const int     save_every = 4;

    auto train_loader = torch::data::make_data_loader(
        std::move(train_ds),
        torch::data::DataLoaderOptions().batch_size(batch_size).workers(16).drop_last(true));
    auto val_loader = torch::data::make_data_loader(
        std::move(val_ds),
        torch::data::DataLoaderOptions().batch_size(batch_size).workers(4).drop_last(false));
    std::cout << "Data loaders done." << std::endl;

    Net model{BOARD_SIZE_NNUE};
    model->to(device);

    torch::optim::AdamW optimizer{model->parameters(),
                                  torch::optim::AdamWOptions(1e-3).weight_decay(1e-4)};
    double              best_loss = std::numeric_limits<double>::infinity();

    const double lr_max = 1e-3;
    const double lr_min = 1e-6;
    std::cout << "Model constructed, starting training..." << std::endl;

    for (int epoch = 1; epoch <= epochs; epoch++) {
        const double lr =
            lr_min + 0.5 * (lr_max - lr_min) * (1.0 + std::cos(M_PI * epoch / epochs));
        for (auto & group : optimizer.param_groups()) {
            static_cast<torch::optim::AdamWOptions &>(group.options()).lr(lr);
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

        model->eval();
        double  sum_sq = 0.0;
        int64_t count = 0;
        {
            torch::NoGradGuard guard;
            for (auto & batch : *val_loader) {
                const auto xb = batch.data.to(device, true);
                const auto yb = batch.target.to(device, true).view({-1, 1});
                const auto pred = model->forward(xb);
                sum_sq += torch::mse_loss(pred, yb, torch::Reduction::Sum).item().to<double>();
                count += yb.size(0);
            }
        }
        const double val_mse = (count > 0) ? sum_sq / static_cast<double>(count) : 0.0;
        std::cout << "epoch " << epoch << " | lr " << lr << " | val mse: " << val_mse << "\n";

        if (epoch % save_every == 0) {
            std::ostringstream name;
            name << "model_epoch_" << std::setw(3) << std::setfill('0') << epoch << ".pt";
            save_module(*model, name.str());
        }
        if (val_mse < best_loss) {
            best_loss = val_mse;
            save_module(*model, "model_best.pt");
        }
    }

    save_module(*model, "model_final.pt");
    return 0;
}
