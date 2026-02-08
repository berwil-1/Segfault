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

static constexpr int board_size = 768;

std::array<float, board_size>
encode_board(const Board & board) {
    std::array<float, board_size> input{};

    constexpr auto pieces = std::array<float, 12>{1.0f,  3.0f,  3.25f,  5.0f,  9.0f,  100.0f,
                                                  -1.0f, -3.0f, -3.25f, -5.0f, -9.0f, -100.0f};

    auto indices = board.occ();

    while (!indices.empty()) {
        const auto index = indices.msb();
        const auto piece = board.at(index);
        const auto piece_value = pieces[static_cast<int>(piece)] / 100.0f;
        const auto psqt_bonus =
            piece_square_table_bonus(board, index, piece.color(), true) / 327.0f;

        const auto do_mobility =
            piece.type() == PieceType::QUEEN || piece.type() == PieceType::ROOK ||
            piece.type() == PieceType::BISHOP || piece.type() == PieceType::KNIGHT;
        const auto mobility =
            do_mobility ? (mobility_bonus(board, index, piece.color(), true) / 116.0f) : 0.0f;

        input[index] = piece_value;
        input[64 + index] = mobility;
        input[128 + index] = psqt_bonus;
        input[192 + index] = board.sideToMove() == chess::Color::WHITE ? 1 : -1;

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
        : seq(torch::nn::Sequential(torch::nn::Linear(input_dim, 64), torch::nn::ReLU(),
                                    torch::nn::Linear(64, 32), torch::nn::ReLU(),
                                    torch::nn::Linear(32, 1))) {
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

    // Device (CPU by default; will use CUDA if available)
    torch::Device device(torch::kCPU);
    if (torch::cuda::is_available()) {
        device = torch::Device(torch::kCUDA);
        std::cout << "CUDA available, training on GPU.\n";
    }

    // 1) Load + encode
    std::cout << "Loading file...\n";
    std::ifstream file("./eval-260205.txt");
    if (!file) {
        throw std::runtime_error("Failed to open ./eval-260205.txt");
    }

    const size_t max_samples = 10'000'000;
    const int    cp_clip = 1200;
    const int    max_fen_repeats = 8;

    std::vector<float> X_flat;
    std::vector<float> y_vec;
    X_flat.reserve(max_samples * static_cast<size_t>(board_size));
    y_vec.reserve(max_samples);

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

        std::string fen(line.data(), sep);

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

        cp = std::clamp(cp, -cp_clip, cp_clip);
        const float score = static_cast<float>(cp) / static_cast<float>(cp_clip);

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
    auto Y = torch::from_blob(y_vec.data(), {N, 1}, torch::TensorOptions().dtype(torch::kFloat32))
                 .clone();

    // Free host vectors (tensors now own copies)
    std::vector<float>{}.swap(X_flat);
    std::vector<float>{}.swap(y_vec);

    X = X.to(device);
    Y = Y.to(device);
    std::cout << "Done\n";

    // 3) Build network
    std::cout << "Building network...\n";
    Net model(board_size);
    model->to(device);

    torch::optim::Adam optimizer(model->parameters(), torch::optim::AdamOptions(3e-4));

    std::cout << "Done\n";

    // 4) Train
    std::cout << "Training...\n";
    const int64_t batch_size = 256;
    const int     epochs = 256;
    const int     save_every = 8;

    float best_loss = std::numeric_limits<float>::infinity();

    for (int epoch = 1; epoch <= epochs; ++epoch) {
        model->train();

        // Shuffle indices each epoch
        auto perm = torch::randperm(N, torch::TensorOptions().dtype(torch::kLong).device(device));

        for (int64_t start = 0; start < N; start += batch_size) {
            const int64_t end = std::min(start + batch_size, N);
            auto          idx = perm.slice(0, start, end);

            auto xb = X.index_select(0, idx);
            auto yb = Y.index_select(0, idx);

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
            torch::NoGradGuard no_grad;
            auto               pred = model->forward(X);
            // Compute sum MSE then divide by N (matches your / X.size() style)
            auto sum = torch::mse_loss(pred, Y, torch::Reduction::Sum);
            val_mse = (sum.item<float>() / static_cast<float>(N));
        }
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
    std::cout << "Done\n";

    return 0;
}
