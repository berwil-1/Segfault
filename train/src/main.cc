// #include "minimal_mlp.hh"
// #include "Eigen/Core"
// #include "MiniDNN/MiniDNN.h"
#include "chess.hh"
#include "process.hh"
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

using namespace chess;
using namespace tiny_dnn;

using PgnType = std::vector<Move>;
using PgnList = std::vector<PgnType>;

/*template<typename T>
std::span<T>
make_subspan(std::span<T> span, size_t offset, size_t count) {
    count = std::min(count, span.size() > offset ? span.size() - offset : 0);
    return span.subspan(offset, count);
}*/

// Convert board to input vector
std::array<float, 64>
encode_board(const Board & board) {
    std::array<float, 64> input{};

    constexpr std::array<float, 12> pieces = {1.0f,  3.0f,  3.25f,  5.0f,  9.0f,  200.0f,
                                              -1.0f, -3.0f, -3.25f, -5.0f, -9.0f, -200.0f};

    auto indices = board.occ();

    while (!indices.empty()) {
        const auto index = indices.msb();
        input[index] = pieces[static_cast<int>(board.at(index))];
        indices.clear(index);
    }

    return input;
}

/*

void
write_process_fen(const Process & proc, const std::string & fen) {
    write_to_process(proc.stdin_fd, "ucinewgame\n");
    write_to_process(proc.stdin_fd, "position fen " + fen + "\n");
    // write_to_process(proc.stdin_fd, "eval\n");
    write_to_process(proc.stdin_fd, "go depth 16\n");
    write_to_process(proc.stdin_fd, "ucinewgame\n");
}

std::string
write_process_uci(std::vector<Move> & moves) {
    std::string out = "";

    // Run stockfish
    auto proc = start_process(
        "/usr/bin/stdbuf",
        {"-o0", "/mnt/c/Users/CoolJWB/Desktop/Programming/chess/stockfish/src/stockfish"});
    write_to_process(proc.stdin_fd, "uci\n");
    write_to_process(proc.stdin_fd, "setoption name Threads value 16");

    write_to_process(proc.stdin_fd, "ucinewgame\n");
    const auto span = std::span<Move>(moves.data(), moves.size());

    std::cout << " Moves: [";
    auto total = std::size_t{0};

    for (auto count = std::size_t{1}; count <= moves.size(); count++) {
        const auto  subspan = std::span(moves.data(), count);
        std::string subspan_moves = "";

        for (auto move : subspan) {
            subspan_moves += uci::moveToUci(move) + " ";
        }

        auto progress = (count * 100 / moves.size());
        if (progress >= total + 10) {
            std::cout << "o";
            total += 10; // step by 10 consistently
        }

        write_to_process(proc.stdin_fd, "position startpos moves " + subspan_moves + "\n");
        write_to_process(proc.stdin_fd, "go depth 16\n");

        if (count % 64 == 0) {
            write_to_process(proc.stdin_fd, "ucinewgame\n");
            write_to_process(proc.stdin_fd, "quit\n", true);
            out += read_from_process(proc.stdout_fd);

            proc = start_process(
                "/usr/bin/stdbuf",
                {"-o0", "/mnt/c/Users/CoolJWB/Desktop/Programming/chess/stockfish/src/stockfish"});
            write_to_process(proc.stdin_fd, "uci\n");
            write_to_process(proc.stdin_fd, "setoption name Threads value 16");
        }
    }
    std::cout << "]" << std::endl;

    write_to_process(proc.stdin_fd, "ucinewgame\n");
    write_to_process(proc.stdin_fd, "quit\n", true);
    out += read_from_process(proc.stdout_fd);

    return out;
}

class MyCounter : public pgn::Visitor {
public:
    virtual ~MyCounter() {}

    void
    startPgn() {
        count++;
    }

    void
    header(std::string_view key, std::string_view value) {}

    void
    startMoves() {}

    void
    move(std::string_view move, std::string_view comment) {}

    void
    endPgn() {}

    std::size_t
    getCount() {
        return count;
    }

private:
    std::size_t count;
    Board       board;
};

class MyVisitor : public pgn::Visitor {
public:
    MyVisitor(std::size_t count) : count_{count} {}

    virtual ~MyVisitor() {}

    void
    startPgn() {
        board_move = board;
        moves.clear();
        boards_move.clear();
    }

    void
    header(std::string_view key, std::string_view value) {}

    void
    startMoves() {}

    void
    move(std::string_view move, std::string_view comment) {
        auto parsed = uci::parseSan(board_move, move);
        moves.push_back(parsed);
        board_move.makeMove(parsed);
        boards_move.push_back(board_move);
    }

    void
    endPgn() {
        std::vector<float> evals;

        setvbuf(stdout, nullptr, _IONBF, 0);
        auto out = write_process_uci(moves);

        std::stringstream ss(out);
        std::string       prev;
        std::string       current;

        auto extract_score = [](const std::string & line) -> int {
            std::istringstream iss(line);
            std::string        token;

            while (iss >> token) {
                if (token == "cp") {
                    int cp;
                    iss >> cp;
                    return cp;
                } else if (token == "mate") {
                    int mate;
                    iss >> mate;
                    return mate > 0 ? 10000 - mate * 100 : -10000 + mate * 100;
                }
            }
            return 0;
        };

        std::ofstream file2("./eval.txt", std::ios::app);
        bool          white = false;

        while (std::getline(ss, current, '\n')) {
            if (prev.starts_with("info") && current.starts_with("bestmove")) {
                auto cp = extract_score(prev);
                auto score = cp / 100.0f;

                file2 << boards_move.at(evals.size()).getFen() << " ; " << (white ? cp : -cp)
                << '\n';
                evals.push_back(white ? score : -score);
                white = !white;
            }

            prev = current;
        }

        file2.close();

        // std::vector<std::array<float, 64>> positions;
        // // std::vector<float>                 evals; // e.g., 0.25 for +0.25 pawns
        // minimal_mlp::MLP net; // default: 64 → 32 → 1

        // for (const auto & board : boards_move) {
        //     positions.push_back(encode_board(board));
        // }

        // net.load("network.bin");
        // net.train(positions, evals, 128, 0.01f);
        // net.save("network.bin");

        // done_++;
        // std::cout << "Progress: " << ((done_ * 100.0f) / count_) << "%" << std::endl;

        // float eval = net.forward(positions.back());
        // std::cout << "Eval: " << eval << " ; "
        //           << "Expected: " << evals.back() << " ; "
        //           << "Delta: " << std::abs(eval - evals.back());
    }

private:
std::size_t count_;
std::size_t done_;

Board              board;
Board              board_move;
std::vector<Move>  moves;
std::vector<Board> boards_move;

// const Board board_default =
//     Board::fromFen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
// static PgnList            pgns;
// static std::vector<Board> boards;
};

*/

using Vec = vec_t; // std::vector<float_t>

// Compute mu/sigma for 64-dim features
struct Norm {
    Vec mu, sigma;
};

Norm
compute_norm(const std::vector<std::array<float, 64>> & boards) {
    const size_t D = 64, N = boards.size();
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
to_batch(const std::vector<std::array<float, 64>> & boards, const std::vector<float> & scores,
         const Norm & norm, size_t i0, size_t i1, std::vector<Vec> & X, std::vector<Vec> & Y) {
    X.clear();
    Y.clear();
    X.reserve(i1 - i0);
    Y.reserve(i1 - i0);
    for (size_t i = i0; i < i1; ++i) {
        Vec x(64);
        for (size_t d = 0; d < 64; ++d)
            x[d] = (boards[i][d] - norm.mu[d]) / norm.sigma[d];
        float y = std::clamp(scores[i] / 12.0f, -1.0f, 1.0f); // scale to [-1,1]
        X.emplace_back(std::move(x));
        Y.emplace_back(Vec{y});
    }
}

int
main() {
    /*auto make_subspan = [](std::span<std::string> span, size_t offset, size_t count) {
        count = std::min(count, span.size() > offset ? span.size() - offset : 0);
        return span.subspan(offset, count);
    };

    std::vector<std::string> fens;
    std::vector<float>       evals;

    std::ifstream file("./fens.txt");
    std::string   line;

    while (std::getline(file, line)) {
        fens.push_back(line);
    }

    setvbuf(stdout, nullptr, _IONBF, 0);

    auto span = std::span<std::string>{fens};

    for (int offset = 0; offset < fens.size(); offset += 8) {
        auto subspan = make_subspan(span, offset, 8);

        // Run stockfish
        auto proc = start_process(
            "/usr/bin/stdbuf",
            {"-o0", "/mnt/c/Users/CoolJWB/Desktop/Programming/chess/stockfish/src/stockfish"});
        write_to_process(proc.stdin_fd, "uci\n");
        write_to_process(proc.stdin_fd, "setoption name Threads value 16");

        for (const auto & fen : subspan) {
            write_process_fen(proc, fen);
        }

        // Quit stockfish
        write_to_process(proc.stdin_fd, "quit\n", true);
        auto out = read_from_process(proc.stdout_fd);

        std::stringstream ss(out);
        std::string       prev;
        std::string       current;

        auto extract_score = [](const std::string & line) -> int {
            std::istringstream iss(line);
            std::string        token;

            while (iss >> token) {
                if (token == "cp") {
                    int cp;
                    iss >> cp;
                    return cp;
                } else if (token == "mate") {
                    int mate;
                    iss >> mate;
                    return mate > 0 ? 10000 - mate * 100 : -10000 + mate * 100;
                }
            }
            return 0;
        };

        std::ofstream file2("./fens-eval.txt", std::ios::app);

        while (std::getline(ss, current, '\n')) {
            if (prev.starts_with("info") && current.starts_with("bestmove")) {
                bool white = fens.at(evals.size()).find(" w ") != std::string::npos;
                auto cp = extract_score(prev);
                auto score = cp / 100.0f;

                file2 << fens.at(evals.size()) << " ; " << (white ? cp : -cp) << '\n';
                evals.push_back(white ? score : -score);
            }

            prev = current;
        }

        file2.close();

        std::cout << "Progress: " << ((offset * 100.0f) / fens.size()) << "%" << std::endl;
    }

    // for (auto index = 0; index < evals.size() / 2; index++) {
    //     std::cout << (index + 1) << ": " << evals.at(index * 2) << " " << evals.at(index * 2 + 1)
    //               << std::endl;
    // }

    std::vector<std::array<float, 64>> positions;
    // std::vector<float>                 evals; // e.g., 0.25 for +0.25 pawns
    minimal_mlp::MLP net; // default: 64 → 32 → 1

    for (const auto & fen : fens) {
        Board board = Board::fromFen(fen);
        positions.push_back(encode_board(board));
    }

    net.load("network.bin");
    net.train(positions, evals, 100, 0.01f);
    net.save("network.bin");

    float eval = net.forward(positions.front());
    std::cout << "Eval = " << eval << "\n";*/

    /*minimal_mlp::MLP net; // default: 64 → 32 → 1
    net.load("network.bin");

    while (true) {
        std::string line;
        std::getline(std::cin, line);

        Board position = Board::fromFen(line);
        float eval = net.forward(encode_board(position));
        std::cout << "Eval = " << eval << "\n";
    }*/

    /*
    // Load PGN
    std::vector<std::string> pgns;
    std::vector<float>       evals;

     std::cout << "Loading file..." << std::endl;
    std::ifstream file("./lichess_db_standard_rated_2016-03.pgn");
    std::string   line;

    std::cout << "Reading PGNs..." << std::endl;
    while (std::getline(file, line)) {
        if (line.starts_with("1. ")) {
            pgns.push_back(line);
        }
    }

    std::cout << pgns.front();*/

    /*
    // Steam the PGN file
    auto file_stream = std::ifstream("./lichess_db_standard_rated_2016-03.pgn");
    // auto file_stream = std::ifstream("./my.pgn");
    // auto file_stream = std::ifstream("./broke.pgn");
    auto vis = std::make_unique<MyVisitor>();

    pgn::StreamParser parser(file_stream);
    auto              error = parser.readGames(*vis);

    if (error) {
        std::cerr << "Error: " << error.message() << "\n";
        return 1;
    }

    auto               span = std::span<PgnType>{vis.get()->getPgns()};
    const auto         boards = vis.get()->getBoards();
    std::vector<float> evals;

    setvbuf(stdout, nullptr, _IONBF, 0);

    // Loop over PGNs
    for (int offset = 0; offset < span.size(); offset += 1) {
        auto subspan = make_subspan(span, offset, 8);

        // Run stockfish
        auto proc = start_process(
            "/usr/bin/stdbuf",
            {"-o0", "/mnt/c/Users/CoolJWB/Desktop/Programming/chess/stockfish/src/stockfish"});
        write_to_process(proc.stdin_fd, "uci\n");
        write_to_process(proc.stdin_fd, "setoption name Threads value 16");

        for (auto & moves : subspan) {
            write_process_uci(proc, moves);
        }

        // Quit stockfish
        write_to_process(proc.stdin_fd, "quit\n", true);
        auto out = read_from_process(proc.stdout_fd);

        std::stringstream ss(out);
        std::string       prev;
        std::string       current;

        auto extract_score = [](const std::string & line) -> int {
            std::istringstream iss(line);
            std::string        token;

            while (iss >> token) {
                if (token == "cp") {
                    int cp;
                    iss >> cp;
                    return cp;
                } else if (token == "mate") {
                    int mate;
                    iss >> mate;
                    return mate > 0 ? 10000 - mate * 100 : -10000 + mate * 100;
                }
            }
            return 0;
        };

        std::ofstream file2("./eval.txt", std::ios::app);
        bool          white = false;

        while (std::getline(ss, current, '\n')) {
            if (prev.starts_with("info") && current.starts_with("bestmove")) {
                auto cp = extract_score(prev);
                auto score = cp / 100.0f;

                file2 << boards.at(evals.size()).getFen() << " ; " << (white ? cp : -cp) << '\n';
                evals.push_back(white ? score : -score);
                white = !white;
            }

            prev = current;
        }

        file2.close();

        std::cout << "Progress: " << ((offset * 100.0f) / span.size()) << "%" << std::endl;
    }

    std::vector<std::array<float, 64>> positions;
    // std::vector<float>                 evals; // e.g., 0.25 for +0.25 pawns
    minimal_mlp::MLP net; // default: 64 → 32 → 1

    for (const auto & board : boards) {
        positions.push_back(encode_board(board));
    }

    net.load("network.bin");
    net.train(positions, evals, 100, 0.01f);
    net.save("network.bin");

    float eval = net.forward(positions.front());
    std::cout << "Eval = " << eval << "\n";*/

    /*// Steam the PGN file
    auto file_stream = std::ifstream("./lichess_db_standard_rated_2013-01.pgn");
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

    std::vector<std::array<float, 64>> positions;
    // std::vector<float>                 evals; // e.g., 0.25 for +0.25 pawns
    minimal_mlp::MLP net; // default: 64 → 32 → 1

    for (const auto & board : boards_move) {
        positions.push_back(encode_board(board));
    }

    net.load("network.bin");
    net.train(positions, evals, 128, 0.01f);
    net.save("network.bin");*/

    /*// Load evals
    std::vector<std::string> fens;
    std::vector<float>       evals;

    std::cout << "Loading file..." << std::endl;
    std::ifstream file("./eval.txt");
    std::string   line;

    std::cout << "Reading evals..." << std::endl;
    while (std::getline(file, line)) {
        auto fen = line.substr(0, line.find(";"));
        auto eval = line.substr(line.find(";") + 2);

        auto cp = std::stoi(eval);
        auto score = cp / 100.0f;

        fens.push_back(fen);
        evals.push_back(score);
    }

    std::vector<std::array<float, 64>> positions;
    // std::vector<float>                 evals; // e.g., 0.25 for +0.25 pawns
    minimal_mlp::MLP net; // default: 64 → 32 → 1

    for (const auto & fen : fens) {
        positions.push_back(encode_board(Board::fromFen(fen)));
    }

    // net.load("network.bin");
    net.train(positions, evals, 128, 0.01f);
    net.save("network.bin");
    float eval = net.forward(positions.front());
    std::cout << "Eval = " << eval << "\n";*/

    /*std::vector<std::array<float, 64>> positions;
    // std::vector<float>                 evals; // e.g., 0.25 for +0.25 pawns
    minimal_mlp::MLP net; // default: 64 → 32 → 1

    net.load("network.bin");

    while (true) {
        std::string fen;
        std::getline(std::cin, fen);

        float eval = net.forward(encode_board(Board::fromFen(fen)));
        std::cout << "Eval = " << eval << "\n";
    }*/

    // 1) Load scores
    std::vector<std::array<float, 64>> boards;
    std::vector<std::string>           fens;
    std::vector<float>                 scores;

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
        boards.push_back(encode_board(Board::fromFen(fen)));
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
    const int    epochs = 64;

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
    std::cout << "Done" << std::endl;

    /*// 1) Load model
    network<sequential> net;
    std::cout << "Load model..." << std::endl;
    net.load("chess_eval_tinydnn.model");
    std::cout << "Done" << std::endl;

    // 2) Predict for one input
    for (;;) {
        std::string line;
        std::getline(std::cin, line);
        std::array<float, 64> sample = encode_board(Board::fromFen(line));
        vec_t                 out = net.predict(vec_t(sample.begin(), sample.end()));
        std::cout << "Prediction: " << out[0] << std::endl;
    }*/
}
