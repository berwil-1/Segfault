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

int
main() {
    // Optional: reproducibility
    torch::manual_seed(1);

    static std::vector<uint64_t> build_offsets_index(const std::string & path,
                                                     std::size_t max_samples, int max_fen_repeats) {
        std::ifstream file(path, std::ios::binary);
        if (!file)
            throw std::runtime_error("Failed to open " + path);

        std::vector<uint64_t> offsets;
        offsets.reserve(max_samples);

        std::unordered_map<std::string, uint8_t> fen_counts;
        fen_counts.reserve(1'000'000);

        std::cout << "Reading + encoding...\n";
        std::string line;
        while (std::getline(file, line) && y_vec.size() < max_samples) {
            const auto sep = line.find(';');
            if (sep == std::string::npos)
                continue;

            size_t eval_pos = sep + 1;
            if (eval_pos < line.size() && line[eval_pos] == ' ')
                ++eval_pos;

            std::string_view fen(line.data(), sep);

            // Allow up to N repeats per identical FEN (fixes the original unordered_set issue)
            auto & cnt = fen_counts[fen];
            if (cnt >= max_fen_repeats)
                continue;
            ++cnt;

            int cp = 0;
            try {
                cp = std::stoi(line.substr(eval_pos));
            } catch (...) {
                continue;
            }

            constexpr auto k = 0.00368208f;
            const auto     score = 1.0f / (1.0f + std::exp(-k * cp));

            Board      b = Board::fromFen(fen);
            const auto enc = encode_board(b);

            X_flat.insert(X_flat.end(), enc.begin(), enc.end());
            y_vec.push_back(score);
        }
        file.close();
        fen_counts.clear();
        std::unordered_map<std::string, uint8_t>{}.swap(fen_counts);

        const int64_t N = static_cast<int64_t>(y_vec.size());
        if (N == 0) {
            throw std::runtime_error("No training samples loaded.");
        }
        std::cout << "Done. Samples: " << N << "\n";

        // 2) Convert to libtorch tensors
        std::cout << "Converting to torch tensors...\n";
        auto X = torch::from_blob(X_flat.data(), {N, static_cast<int64_t>(board_size)},
                                  torch::TensorOptions().dtype(torch::kFloat32))
                     .clone(); // own the memory
        auto Y =
            torch::from_blob(y_vec.data(), {N, 1}, torch::TensorOptions().dtype(torch::kFloat32))
                .clone();

        torch::Device device{torch::kCPU};
        if (torch::cuda::is_available()) {
            device = torch::Device{torch::kCUDA};
            std::cout << "CUDA available, training on GPU.\n";
        }

        const std::string path = "./eval-260205.txt";
        const size_t      max_samples = 1'000'000;
        const int         cp_clip = 1200;
        const int         max_fen_repeats = 8;

        std::cout << "Indexing offsets...\n";
        auto offsets = build_offsets_index(path, max_samples, max_fen_repeats);
        if (offsets.empty())
            throw std::runtime_error("No samples indexed.");
        std::cout << "Done. Indexed samples: " << offsets.size() << "\n";

        std::mt19937_64 rng(1);
        std::shuffle(offsets.begin(), offsets.end(), rng);

        // then split
        const size_t          val_n = std::min<size_t>(1'000'000, offsets.size() / 20);
        std::vector<uint64_t> val_offsets(offsets.begin(), offsets.begin() + val_n);
        std::vector<uint64_t> train_offsets(offsets.begin() + val_n, offsets.end());

        auto train_ds = FenEvalDataset(path, std::move(train_offsets), cp_clip)
                            .map(torch::data::transforms::Stack<>());
        auto val_ds = FenEvalDataset(path, std::move(val_offsets), cp_clip)
                          .map(torch::data::transforms::Stack<>());

        const int64_t batch_size = 256;
        const int     epochs = 256;
        const int     save_every = 8;

        auto train_loader = torch::data::make_data_loader(
            std::move(train_ds),
            torch::data::DataLoaderOptions()
                .batch_size(batch_size)
                .workers(4) // tune; 0 if you hit I/O issues on Windows/WSL
                .drop_last(true));

        auto val_loader = torch::data::make_data_loader(
            std::move(val_ds),
            torch::data::DataLoaderOptions().batch_size(batch_size).workers(2).drop_last(false));

        // 3) Build network
        std::cout << "Building network...\n";
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

                // MSE (mean) per batch
                auto loss = torch::mse_loss(pred, yb, torch::Reduction::Mean);
                loss.backward();
                optimizer.step();
            }

            // Validation MSE on the full dataset (matches your callback intent)
            model->eval();
            float val_mse = 0.0f;
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

            // Periodic checkpoint
            if (epoch % save_every == 0) {
                std::ostringstream name;
                name << "model_epoch_" << std::setw(3) << std::setfill('0') << epoch << ".pt";
                save_module(*model, name.str());
            }

            // Best checkpoint
            if (val_mse < best_loss) {
                best_loss = val_mse;
                save_module(*model, "model_best.pt");
            }
        }
        std::cout << "Done\n";

        // 5) Save final model
        std::cout << "Save model...\n";
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
