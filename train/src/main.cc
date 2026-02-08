#include "chess.hh"
#include "eval.hh"
#include "process.hh"
#include "stockfish.hh"
#include "tiny_dnn/tiny_dnn.h"

#include <array>
#include <chrono>
#include <cstdint>
#include <ios>
#include <iostream>
#include <memory>
#include <ranges>
#include <span>
#include <sstream>
#include <string>
#include <unordered_set>

using namespace chess;
using namespace tiny_dnn;
using namespace segfault;

using PgnType = std::vector<Move>;
using PgnList = std::vector<PgnType>;

constexpr auto board_size = 256;

// Convert board to input vector
std::array<float, board_size>
encode_board(const Board & board) {
    std::array<float, board_size> input{};

    constexpr auto pieces = std::array<float, 12>{
        1.0f, 3.0f, 3.25f, 5.0f, 9.0f, 100.0f,
        -1.0f, -3.0f, -3.25f, -5.0f, -9.0f, -100.0f
    };

    auto indices = board.occ();

    while (!indices.empty()) {
        const auto index = indices.msb();
        const auto piece = board.at(index);
        const auto piece_value = pieces[static_cast<int>(piece)] / 100.0f;
        const auto psqt_bonus = piece_square_table_bonus(board, index, piece.color(), true) / 327.0f;

        const auto do_mobility = piece.type() == PieceType::QUEEN || piece.type() == PieceType::ROOK || 
            piece.type() == PieceType::BISHOP || piece.type() == PieceType::KNIGHT;
        const auto mobility = do_mobility ? (mobility_bonus(board, index, piece.color(), true) / 116.0f) : 0.0f;

        input[index] = piece_value;
        input[64 + index] = mobility;
        input[128 + index] = psqt_bonus;
        input[192 + index] = board.sideToMove() == chess::Color::WHITE ? 1 : -1;

        indices.clear(index);
    }

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
to_batch(const std::vector<std::array<float, board_size>> & boards, const std::vector<float> & scores,
         const Norm & norm, size_t i0, size_t i1, std::vector<Vec> & X, std::vector<Vec> & Y) {
    X.clear();
    Y.clear();
    X.reserve(i1 - i0);
    Y.reserve(i1 - i0);
    for (size_t i = i0; i < i1; ++i) {
        Vec x(board_size);
        for (size_t d = 0; d < board_size; ++d)
            x[d] = (boards[i][d] - norm.mu[d]) / norm.sigma[d];
        //float y = std::clamp(scores[i] / 12.0f, -1.0f, 1.0f); // scale to [-1,1]
        float y = scores[i];
        X.emplace_back(std::move(x));
        Y.emplace_back(Vec{y});
    }
}

int
main() {
    /*// Steam the PGN file
    auto file_stream = std::ifstream("./lichess_db_standard_rated_2017-03.pgn");
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
    }*/

    // 1) Load scores
    std::vector<std::array<float, board_size>> boards;
    std::vector<float>              scores;
    std::unordered_set<std::string> fens;

    std::cout << "Loading file...\n";
    std::ifstream file("./fen_cp.txt");
    if (!file) {
        throw std::runtime_error("Failed to open ./fen_cp.txt");
    }

    std::string line;
    std::cout << "Reading + encoding...\n";

    while (std::getline(file, line) && scores.size() < 10000000) {
        const auto sep = line.find(';');
        if (sep == std::string::npos) {
            continue;
        }

        // Expected format: "FEN ; cp", eval starts 
        // after "; " (2 chars) if present
        size_t eval_pos = sep + 1;
        if (eval_pos < line.size() && line[eval_pos] == ' ') {
            ++eval_pos;
        }

        std::string fen(line.data(), sep);
        if (fens.count(fen) > 8) {
            continue;
        }
        fens.emplace(fen);

        int cp = 0;
        try {
            cp = std::stoi(std::string(line.substr(eval_pos)));
        } catch (...) {
            continue;
        }

        cp = std::clamp(cp, -1200, 1200);
        const float score = static_cast<float>(cp) / 1200.0f;
        Board board = Board::fromFen(std::string(fen));

        /*if (board.fullMoveNumber() > 10) {
            continue;
        }*/

        boards.push_back(encode_board(board));
        scores.push_back(score);
    }
    fens.clear();
    std::unordered_set<std::string>{}.swap(fens);
    file.close();
    std::cout << "Done\n";

    // 2) Convert to tiny-dnn format
    std::cout << "Converting to tiny-dnn format..." << std::endl;
    std::vector<tiny_dnn::vec_t> X, Y;
    X.reserve(boards.size());
    Y.reserve(boards.size());

    for (size_t i = 0; i < boards.size(); ++i) {
        X.emplace_back(boards[i].begin(), boards[i].end());
        Y.emplace_back(tiny_dnn::vec_t{scores[i]});
    }
    std::vector<std::array<float, board_size>>{}.swap(boards);
    std::vector<tiny_dnn::float_t>{}.swap(scores);
    std::cout << "Done" << std::endl;

    // 3) Build network
    std::cout << "Building network..." << std::endl;
    const size_t        input_dimensions = X.front().size();
    tiny_dnn::network<tiny_dnn::sequential> net;
    net << tiny_dnn::fully_connected_layer(input_dimensions, 64) << tiny_dnn::relu_layer()
    << tiny_dnn::fully_connected_layer(64, 32) << tiny_dnn::relu_layer()
    << tiny_dnn::fully_connected_layer(32, 1);

    tiny_dnn::adam optimizer;
    optimizer.alpha = 3e-4f;

    std::cout << "Done" << std::endl;

    // 4) Train
    std::cout << "Training..." << std::endl;
    const size_t batch_size = 256;
    const int    epochs = 256;

    int       epoch_idx = 0;
    float     best_loss = std::numeric_limits<float>::infinity();
    const int save_every = 8;

    net.fit<tiny_dnn::mse>(
        optimizer, X, Y, batch_size, epochs, [] {}, // minibatch callback
        [&] {
            ++epoch_idx;

            net.set_netphase(tiny_dnn::net_phase::test);
            const float loss = static_cast<float>(net.get_loss<mse>(X, Y) / X.size());
            net.set_netphase(tiny_dnn::net_phase::train);
            std::cout << "val mse: " << loss << "\n";

            // Periodic checkpoint
            if (epoch_idx % save_every == 0) {
                std::ostringstream name;
                name << "model_epoch_" << std::setw(3) << std::setfill('0') << epoch_idx
                     << ".model";
                net.save(name.str());
            }

            // Best checkpoint
            if (loss < best_loss) {
                best_loss = loss;
                net.save("model_best.model");
            }
        });
    std::cout << "Done" << std::endl;

    // 5) Save model
    std::cout << "Save model..." << std::endl;
    net.save("model_final.model");
    std::cout << "Done" << std::endl;

    /*// 1) Load model
    network<sequential> net;
    std::cout << "Load model..." << std::endl;
    net.load("model_best_6.model");
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
