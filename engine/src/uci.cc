#include "uci.hh"

#include "eval.hh"
#include "search.hh"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <span>
#include <sstream>
#include <vector>

namespace {
auto
string_split(std::string line, char delimiter) {
    std::vector<std::string> args;
    std::stringstream        stream(line);
    std::string              arg;

    while (std::getline(stream, arg, delimiter)) {
        args.push_back(arg);
    }

    return args;
}
} // namespace

namespace segfault {

using namespace chess;

void
Uci::start() {
    std::string line;

    active_ = true;
    search_stop_ = false;
    search_done_ = false;

    uci();

    while (active_) {
        std::getline(std::cin, line);

        if (search_done_ && search_thread_.joinable()) {
            search_stop_ = false;
            search_done_ = false;
            search_thread_.join();
        }

        const auto args = string_split(line, ' ');
        auto       it = commands.find(args.front());

        if (it != commands.end()) {
            it->second(line);
        } else {
            std::cout << "Unknown command: \'" << line << "\'." << std::endl;
        }
    }

    if (search_thread_.joinable()) {
        search_stop_ = false;
        search_done_ = false;
        search_thread_.join();
    }
}

std::vector<std::string>
Uci::getMoves() {
    return moves_;
}

std::string
Uci::getStartFen() {
    return startpos_;
}

void
Uci::uci() {
    std::cout << "id name segfault\n"
              << "id author berwil-1\n"
              << "uciok" << std::endl;
}

void
Uci::isready() {
    std::cout << "readyok" << std::endl;
}

void
Uci::ucinewgame() {
    moves_.clear();
    startpos_ = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
    board_ = Board::fromFen(chess::constants::STARTPOS);
}

void
Uci::position(const std::string & command) {
    const auto args = string_split(command, ' ');

    if (args.size() > 1) {
        if (args.at(1) == "startpos") {
            startpos_ = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
            board_ = Board::fromFen(startpos_);
        }
    }

    if (args.size() > 2) {
        if (args.at(1) == "fen") {
            startpos_ = "";

            for (std::size_t index = 2; index < args.size(); index++) {
                startpos_.append(args.at(index) + " ");
            }

            board_ = Board::fromFen(startpos_);
        }

        const auto it = std::find(args.begin(), args.end(), "moves");
        if (it != args.end()) {
            for (auto move_it = it + 1; move_it != args.end(); ++move_it) {
                moves_.push_back(*move_it);
                // std::cout << *move_it << ": " << board_.getFen() << "\n";
                board_.makeMove(uci::uciToMove(board_, *move_it));
            }
        }
    }
}

void
Uci::go(const std::string & command, std::atomic<bool> & stop) {
    const auto args = string_split(command, ' ');
    auto       parameters = std::unordered_map<std::string, std::string>{};

    for (auto i = std::size_t{1}; i < args.size(); i++) {
        const auto & key = args.at(i);
        if (i + 1 < args.size() && key != "infinite") {
            parameters[key] = args.at(i + 1);
            i++;
        } else {
            parameters[key] = "";
        }
    }

    auto bestmove = chess::Move{};

    if (parameters.contains("depth")) {
        const auto depth = static_cast<int>(std::stoi(parameters.at("depth")));
        bestmove = segfault_.search(board_, depth, stop);
    } else if (parameters.contains("infinite")) {
        bestmove = segfault_.search(board_, 255, stop);
    } else {
        const auto wtime =
            std::stoull(parameters.contains("wtime") ? parameters.at("wtime") : "60000");
        const auto btime =
            std::stoull(parameters.contains("btime") ? parameters.at("btime") : "60000");
        const auto winc = std::stoull(parameters.contains("winc") ? parameters.at("winc") : "1000");
        const auto binc = std::stoull(parameters.contains("binc") ? parameters.at("binc") : "1000");

        bestmove = segfault_.search(board_, wtime, btime, winc, binc, stop);
    }

    board_.makeMove(bestmove);
    search_done_ = true;

    std::cout << "bestmove " << uci::moveToUci(bestmove) << std::endl;
}

void
Uci::stop() {
    search_stop_ = true;
}

void
Uci::debug(const std::string & command) {
    constexpr std::array<std::string_view, 13> symbols = {
        "♟", // WHITE PAWN
        "♞", // WHITE KNIGHT
        "♝", // WHITE BISHOP
        "♜", // WHITE ROOK
        "♛", // WHITE QUEEN
        "♚", // WHITE KING
        "♙", // BLACK PAWN
        "♘", // BLACK KNIGHT
        "♗", // BLACK BISHOP
        "♖", // BLACK ROOK
        "♕", // BLACK QUEEN
        "♔", // BLACK KING
        " " // NONE
    };

    constexpr std::string_view LIGHT_BG = "\033[48;5;236m"; // dark gray
    constexpr std::string_view DARK_BG = "\033[48;5;234m"; // almost black
    constexpr std::string_view RESET = "\033[0m";

    const auto args = string_split(command, ' ');

    if (args.size() > 1) {
        if (args.at(1) == "board") {
            for (auto y = 0; y < 8; y++) {
                for (auto x = 0; x < 8; x++) {
                    if (((y * 8 + x + y) & 1) == 0)
                        std::cout << DARK_BG;
                    else
                        std::cout << LIGHT_BG;

                    std::cout << " " << symbols[static_cast<int>(board_.at(y * 8 + x))] << " ";

                    std::cout << RESET;
                }
                std::cout << std::endl;
            }
        } else if (args.at(1) == "eval") {
            std::cout << "Eval Stockfish: " << evaluateStockfish(board_, false) << std::endl;
            std::cout << "Eval Segfault: " << evaluateSegfault(board_) << std::endl;
            // std::cout << "Eval Network: " << evaluateNetwork(board_) << std::endl;
        }
    }
}

void
Uci::quit() {
    active_ = false;
}

} // namespace segfault
