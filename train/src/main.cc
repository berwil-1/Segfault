#include "chess.hh"
#include "eval.hh"
#include "process.hh"
#include "stockfish.hh"
#include "tiny_dnn/tiny_dnn.h"

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
#include <string>

using namespace chess;
using namespace segfault;

static constexpr int board_size = 320;

constexpr auto board_size = 65;

// Convert board to input vector
std::array<float, board_size>
encode_board(const Board & board) {
    std::array<float, board_size> input{};

    constexpr std::array<float, 12> pieces = {1.0f,  3.0f,  3.25f,  5.0f,  9.0f,  100.0f,
                                              -1.0f, -3.0f, -3.25f, -5.0f, -9.0f, -100.0f};

    auto indices = board.occ();

    while (!indices.empty()) {
        const auto index = indices.msb();
        input[index] = pieces[static_cast<int>(board.at(index))] / 100.0f;
        indices.clear(index);
    }
    input[64] = board.sideToMove() == chess::Color::WHITE ? 1 : -1;

    return input;
}

using Vec = vec_t; // std::vector<float_t>

// Compute mu/sigma for 64-dim features
struct Norm {
    Vec mu, sigma;
};

Norm
compute_norm(const std::vector<std::array<float, board_size>> & boards) {
    const size_t D = board_size, N = boards.size();
    Vec          mu(D, 0), m2(D, 0);
    for (const auto & b : boards)
        for (size_t d = 0; d < D; ++d) {
            float x = b[d];
            float delta = x - mu[d];
            mu[d] += delta / float(N);
            m2[d] += delta * (x - mu[d]);
        }
    Vec sigma(D, 1);
    for (size_t d = 0; d < D; ++d) {
        float var = (N > 1) ? m2[d] / float(N - 1) : 1.f;
        sigma[d] = std::max(1e-6f, std::sqrt(var));
    }
    return {mu, sigma};
}

// Convert to tiny-dnn batch
void
to_batch(const std::vector<std::array<float, board_size>> & boards,
         const std::vector<float> & scores, const Norm & norm, size_t i0, size_t i1,
         std::vector<Vec> & X, std::vector<Vec> & Y) {
    X.clear();
    Y.clear();
    X.reserve(i1 - i0);
    Y.reserve(i1 - i0);
    for (size_t i = i0; i < i1; ++i) {
        Vec x(board_size);
        for (size_t d = 0; d < board_size; ++d)
            x[d] = (boards[i][d] - norm.mu[d]) / norm.sigma[d];
        // float y = std::clamp(scores[i] / 12.0f, -1.0f, 1.0f); // scale to [-1,1]
        float y = scores[i];
        X.emplace_back(std::move(x));
        Y.emplace_back(Vec{y});
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
    // Steam the PGN file
    auto file_stream = std::ifstream("./lichess_db_standard_rated_2020-01.pgn");
    // auto file_stream = std::ifstream("./my.pgn");
    // auto file_stream = std::ifstream("./broke.pgn");
    auto cnt = std::make_unique<MyCounter>();

    pgn::StreamParser parser_cnt(file_stream);
    auto              error = parser_cnt.readGames(*cnt);

    if (error) {
        std::cerr << "Error counter: " << error.message() << "\n";
        return 1;
    }

    const auto count = cnt.get()->getCount();
    std::cout << "Total count: " << count << "\n";
    auto vis = std::make_unique<MyVisitor>(count);

    file_stream.clear();
    file_stream.seekg(0, std::ios::beg);
    pgn::StreamParser parser_vis(file_stream);
    error = parser_vis.readGames(*vis);

    if (error) {
        std::cerr << "Error visitor: " << error.message() << "\n";
        return 1;
    }

    /*// 1) Load scores
    std::vector<std::array<float, board_size>> boards;
    //std::vector<std::vector<float>> boards;
    std::vector<std::string>        fens;
    std::vector<float>              scores;

    std::cout << "Loading file..." << std::endl;
    std::ifstream file("./eval-full.txt");
    std::string   line;

    std::cout << "Reading scores..." << std::endl;
    while (std::getline(file, line)) {
        auto fen = line.substr(0, line.find(";"));
        auto eval = line.substr(line.find(";") + 2);

        int cp = std::stoi(eval);
        cp = std::clamp(cp, -1200, 1200); // avoid mate/outlier skew
        float score = cp / 1200.0f; // scale to [-1, 1]
        // auto score = cp / 100.0f;

        fens.push_back(fen);
        scores.push_back(score);
    }

    for (const auto & fen : fens) {
        auto encoded = encode_board(Board::fromFen(fen));
        boards.push_back(encoded);
    }
    std::cout << "Done" << std::endl;

    // 2) Convert to tiny-dnn format
    std::cout << "Converting to tiny-dnn format..." << std::endl;
    std::vector<vec_t> X, Y;
    X.reserve(boards.size());
    Y.reserve(boards.size());
    for (size_t i = 0; i < boards.size(); i++) {
        X.emplace_back(boards[i].begin(), boards[i].end());
        Y.emplace_back(vec_t{scores[i]});
    }
    std::cout << "Done" << std::endl;

    // 3) Build network
    std::cout << "Building network..." << std::endl;
    const size_t        input_dimensions = X.front().size();
    network<sequential> net;
    net << fully_connected_layer(input_dimensions, 256) << relu_layer() << dropout_layer(256, 0.2f)
        << fully_connected_layer(256, 128) << relu_layer() << dropout_layer(128, 0.2f)
        << fully_connected_layer(128, 1);

    adam optimizer;
    optimizer.alpha = 1e-4f;

    std::cout << "Done" << std::endl;

    // 4) Train
    std::cout << "Training..." << std::endl;
    const size_t batch_size = 256;
    const int    epochs = 128;

    int       epoch_idx = 0;
    float     best_loss = std::numeric_limits<float>::infinity();
    const int save_every = 8;

    net.fit<mse>(
        optimizer, X, Y, batch_size, epochs, [] {}, // minibatch callback
        [&] {
            ++epoch_idx;

            // evaluate
            net.set_netphase(tiny_dnn::net_phase::test);
            const float loss = static_cast<float>(net.get_loss<mse>(X, Y) / X.size());
            net.set_netphase(tiny_dnn::net_phase::train);
            std::cout << "val mse: " << loss << "\n";

            // periodic checkpoint
            if (epoch_idx % save_every == 0) {
                std::ostringstream name;
                name << "model_epoch_" << std::setw(3) << std::setfill('0') << epoch_idx
                     << ".model";
                net.save(name.str()); // saves arch + weights (binary)
            }

            // best checkpoint
            if (loss < best_loss) {
                best_loss = loss;
                net.save("model_best.model");
            }
        });
    std::cout << "Done" << std::endl;

    // 5) Save model
    std::cout << "Save model..." << std::endl;
    net.save("model_final.model");
    std::cout << "Done" << std::endl;*/

    /*// 1) Load model
    network<sequential> net;
    std::cout << "Load model..." << std::endl;
    net.load("model_best.model");
    std::cout << "Done" << std::endl;

    // 2) Predict for one input
    for (;;) {
        std::string line;
        std::getline(std::cin, line);
        std::array<float, board_size> sample = encode_board(Board::fromFen(line));
        vec_t                 out = net.predict(vec_t(sample.begin(), sample.end()));
        std::cout << "Prediction: " << out[0] << std::endl;
    }*/
}
