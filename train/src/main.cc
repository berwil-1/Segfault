#include "chess.hh"
#include "minimal_mlp.hh"
#include "process.hh"

#include <array>
#include <cstdint>
#include <iostream>
#include <memory>
#include <ranges>
#include <span>
#include <sstream>

using namespace chess;

using PgnType = std::pair<Board, std::vector<Move>>;
using PgnList = std::vector<PgnType>;

template<typename T>
std::span<T>
make_subspan(std::span<T> span, size_t offset, size_t count) {
    count = std::min(count, span.size() > offset ? span.size() - offset : 0);
    return span.subspan(offset, count);
}

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

void
write_process_fen(const Process & proc, const std::string & fen) {
    write_to_process(proc.stdin_fd, "ucinewgame\n");
    write_to_process(proc.stdin_fd, "position fen " + fen + "\n");
    // write_to_process(proc.stdin_fd, "eval\n");
    write_to_process(proc.stdin_fd, "go depth 20\n");
    write_to_process(proc.stdin_fd, "ucinewgame\n");
}

void
write_process_uci(const Process & proc, const Board & board, std::vector<Move> & moves) {
    // write_to_process(proc.stdin_fd, "ucinewgame\n");
    // write_to_process(proc.stdin_fd, "position fen " + fen + "\n");
    //  write_to_process(proc.stdin_fd, "eval\n");

    write_to_process(proc.stdin_fd, "ucinewgame\n");
    const auto span = std::span<Move>{moves};

    for (auto count = 1; count <= moves.size(); count++) {
        const auto  subspan = make_subspan<Move>(span, 0, count);
        std::string subspan_moves = "";

        for (auto move : subspan) {
            subspan_moves += uci::moveToUci(move) + " ";
        }

        write_to_process(proc.stdin_fd,
                         "position fen " + board.getFen() + " moves " + subspan_moves + "\n");
        // std::cout << ("position " + board.getFen() + " moves " + subspan_moves + "\n");
        write_to_process(proc.stdin_fd, "go depth 20\n");
    }

    write_to_process(proc.stdin_fd, "ucinewgame\n");
}

class MyVisitor : public pgn::Visitor {
public:
    virtual ~MyVisitor() {}

    void
    startPgn() {
        pgns.emplace_back(board, std::vector<Move>{});
    }

    void
    header(std::string_view key, std::string_view value) {}

    void
    startMoves() {}

    void
    move(std::string_view move, std::string_view comment) {
        auto & last = pgns.back().second;
        auto   parsed = uci::parseSan(board_moves, move);
        board_moves.makeMove(parsed);
        // std::cout << boards.size() << ": " << move << std::endl;
        boards.push_back(board_moves);
        // std::cout << last.size() << ": " << parsed << std::endl;
        last.push_back(parsed);
        std::cout << board_moves.getFen() << ": " << move << std::endl;
    }

    void
    endPgn() {}

    PgnList &
    getPgns() {
        return pgns;
    }

    std::vector<Board> &
    getBoards() {
        return boards;
    }

private:
    Board                     board;
    Board                     board_moves;
    static PgnList            pgns;
    static std::vector<Board> boards;
};

PgnList            MyVisitor::pgns;
std::vector<Board> MyVisitor::boards;

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

    /*Board position = Board::fromFen("2r5/p3kp2/b3p3/5p1P/1P2R3/P2pQ3/5PPK/2qB4 w - - 0 34");

    minimal_mlp::MLP net; // default: 64 → 32 → 1
    net.load("network.bin");

    float eval = net.forward(encode_board(position));
    std::cout << "Eval = " << eval << "\n";*/

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

    // Steam the PGN file
    // auto file_stream = std::ifstream("./lichess_db_standard_rated_2016-03.pgn");
    auto file_stream = std::ifstream("./my.pgn");
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

        for (auto & pgn : subspan) {
            write_process_uci(proc, pgn.first, pgn.second);
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
    std::cout << "positions: " << positions.size() << std::endl;
    std::cout << "evals: " << evals.size() << std::endl;
    net.train(positions, evals, 100, 0.01f);
    net.save("network.bin");

    float eval = net.forward(positions.front());
    std::cout << "Eval = " << eval << "\n";
}
