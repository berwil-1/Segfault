#include "chess.hh"
#include "eval.hh"
#include "process.hh"
#include "stockfish.hh"
#include "torch/torch.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <random>
#include <rocksdb/db.h>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

using namespace chess;
using namespace segfault;

static constexpr int board_size = 320;

std::array<float, board_size>
encode_board(const Board & board) {
    std::array<float, board_size> input{};

    constexpr auto pieces = std::array<float, 12>{1.0f,  3.0f,  3.25f,  5.0f,  9.0f,  100.0f,
                                                  -1.0f, -3.0f, -3.25f, -5.0f, -9.0f, -100.0f};

    auto indices = board.occ();

    while (!indices.empty()) {
        const auto index = indices.msb();
        const auto piece = board.at(index);
        const auto piece_value =
            1.0f / (1.0f + std::exp(-0.06f * 2.0f * pieces[static_cast<int>(piece)]));
        const auto psqt_bonus =
            1.0f /
            (1.0f + std::exp(-0.02f * piece_square_table_bonus(board, index, piece.color(), true)));

        const auto do_mobility =
            piece.type() == PieceType::QUEEN || piece.type() == PieceType::ROOK ||
            piece.type() == PieceType::BISHOP || piece.type() == PieceType::KNIGHT;
        const auto mobility =
            do_mobility
                ? (1.0f /
                   (1.0f + std::exp(-0.05f * mobility_bonus(board, index, piece.color(), true))))
                : 0.0f;

        input[index] = piece_value;
        input[64 + index] = mobility;
        input[128 + index] = psqt_bonus;
        input[192 + index] = board.sideToMove() == chess::Color::WHITE ? 1 : -1;
        input[256 + index] = 1.0f / (1.0f + std::exp(-0.1f * (board.fullMoveNumber() - 50)));

        indices.clear(index);
    }
    input[64] = board.sideToMove() == chess::Color::WHITE ? 1 : -1;

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
        : seq(torch::nn::Sequential(torch::nn::Linear(input_dim, 256), torch::nn::ReLU(),
                                    torch::nn::Linear(256, 128), torch::nn::ReLU(),
                                    torch::nn::Linear(128, 1))) {
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

static std::vector<std::string>
build_keys(rocksdb::DB * const database, std::size_t max_samples) {
    auto *                   it = database->NewIterator(rocksdb::ReadOptions());
    std::vector<std::string> keys;

    for (it->SeekToFirst(); it->Valid(); it->Next()) {
        if (keys.size() >= max_samples) {
            break;
        }
        keys.emplace_back(it->key().ToString());
    }

    delete it;
    return keys;
}

struct FenEvalDataset : torch::data::datasets::Dataset<FenEvalDataset> {
    rocksdb::DB *            database_;
    std::vector<std::string> keys_;

    FenEvalDataset(rocksdb::DB * const database, const std::vector<std::string> && keys)
        : database_(database), keys_(std::move(keys)) {}

    torch::data::Example<>
    get(std::size_t index) override {
        std::string value;
        const auto  fen = keys_[index];
        database_->Get(rocksdb::ReadOptions(), fen, &value);

        int cp = 0;
        try {
            cp = std::stoi(value);
        } catch (...) {
            cp = 0;
        }

        constexpr auto k = 0.00368208f;
        const auto     score = 1.0f / (1.0f + std::exp(-k * cp));

        Board      board = Board::fromFen(fen);
        const auto enc = encode_board(board); // std::array<float, board_size>

        auto x = torch::from_blob((void *)enc.data(), {static_cast<int64_t>(board_size)},
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

    torch::Device device{torch::kCPU};
    if (torch::cuda::is_available()) {
        device = torch::Device{torch::kCUDA};
        std::cout << "CUDA available, training on GPU.\n";
    }

    constexpr auto    max_samples = 50'000'000;
    const std::string path = "./fens";
    rocksdb::DB *     database;
    rocksdb::Options  options;

    rocksdb::DB::Open(options, path, &database);
    auto keys = build_keys(database, max_samples);

    if (keys.empty())
        throw std::runtime_error("No samples indexed.");
    std::cout << "Done. Indexed samples: " << keys.size() << "\n";

    std::mt19937_64 rng{1};
    std::shuffle(keys.begin(), keys.end(), rng);

    // Do training and value split
    const std::size_t        val_n = std::min<size_t>(1'000'000, keys.size() / 20);
    std::vector<std::string> val_keys{keys.begin(), keys.begin() + val_n};
    std::vector<std::string> train_keys{keys.begin() + val_n, keys.end()};

    auto train_ds =
        FenEvalDataset(database, std::move(train_keys)).map(torch::data::transforms::Stack<>());
    auto val_ds =
        FenEvalDataset(database, std::move(val_keys)).map(torch::data::transforms::Stack<>());

    const int64_t batch_size = 256;
    const int     epochs = 256;
    const int     save_every = 8;

    auto train_loader = torch::data::make_data_loader(
        std::move(train_ds),
        torch::data::DataLoaderOptions().batch_size(batch_size).workers(4).drop_last(true));

    auto val_loader = torch::data::make_data_loader(
        std::move(val_ds),
        torch::data::DataLoaderOptions().batch_size(batch_size).workers(2).drop_last(false));

    Net model(board_size);
    model->to(device);

    // torch::optim::Adam optimizer(model->parameters(), torch::optim::AdamOptions(3e-4));
    torch::optim::AdamW optimizer{model->parameters(),
                                  torch::optim::AdamWOptions(3e-4).weight_decay(1e-4)};
    float               best_loss = std::numeric_limits<float>::infinity();

    for (int epoch = 1; epoch <= epochs; ++epoch) {
        model->train();

        for (auto & batch : *train_loader) {
            auto xb = batch.data.to(device, true);
            auto yb = batch.target.to(device, true).view({-1, 1});

            optimizer.zero_grad();
            auto pred = model->forward(xb);
            auto loss = torch::mse_loss(pred, yb, torch::Reduction::Mean);
            loss.backward();
            optimizer.step();
        }

        // streamed validation MSE (sum / count)
        model->eval();
        double  sum_sq = 0.0;
        int64_t count = 0;
        {
            torch::NoGradGuard ng;
            for (auto & batch : *val_loader) {
                auto   xb = batch.data.to(device, true);
                auto   yb = batch.target.to(device, true).view({-1, 1});
                auto   pred = model->forward(xb);
                double s = torch::mse_loss(pred, yb, torch::Reduction::Sum).item().to<double>();
                sum_sq += s;
                count += yb.size(0);
            }
        }
        const double val_mse = (count > 0) ? static_cast<double>(sum_sq / (double)count) : 0.0f;
        std::cout << "epoch " << epoch << " | val mse: " << val_mse << "\n";

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

    /*torch::Device device(torch::kCPU);
    if (torch::cuda::is_available())
        device = torch::kCUDA; // optional

    // 1) Load model weights
    Net model(board_size);
    load_module(*model, "model_best.pt");
    model->to(device);
    model->eval();

    std::cout << "Loaded model_best.pt. Enter FEN lines:\n";

    // 2) Predict for one input repeatedly
    for (std::string line; std::getline(std::cin, line);) {
        if (line.empty())
            continue;

        const auto enc = encode_board(Board::fromFen(line));

        // shape: [1, board_size]
        auto x = torch::from_blob((void *)enc.data(), {1, board_size},
                                  torch::TensorOptions().dtype(torch::kFloat32))
                     .clone()
                     .to(device);

        torch::NoGradGuard no_grad;
        auto               y = model->forward(x); // [1, 1]
        float              pred = y.item<float>(); // roughly in [-1, 1] given your training
    targets

        std::cout << "Prediction: " << pred;

        // Optional: convert back to centipawns with the same scale you used in training
        //float cp_est = pred * 1200.0f;
        constexpr auto k = 0.00368208f;
        float cp_est = std::log((1 / pred) - 1) / -k;
        std::cout << " (≈ " << cp_est << " cp)" << "\n";
    }*/

    return 0;
}
