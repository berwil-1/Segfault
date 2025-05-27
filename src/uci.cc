#include "uci.hh"

#include <algorithm>
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
    uci();

    while (active_) {
        std::getline(std::cin, line);

        if (search_thread_.joinable())
            search_thread_.join();

        const auto args = string_split(line, ' ');
        auto       it = commands.find(args.front());

        if (it != commands.end()) {
            it->second(line);
        } else {
            std::cout << "Unknown command: \'" << line << "\'." << std::endl;
        }
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
Uci::setCallback(Callback func) {
    callback_ = func;
}

void
Uci::uci() {
    std::cout << "id name segfault\n"
              << "id author William Bergh\n"
              << "uciok" << std::endl;
}

void
Uci::isready() {
    std::cout << "readyok" << std::endl;
}

void
Uci::ucinewgame() {
    moves_.clear();
    startpos_ = "";
    board_ = Board::fromFen(startpos_);
}

void
Uci::position(const std::string & command) {
    const auto args = string_split(command, ' ');

    if (args.size() > 2) {
        if (args.at(2) == "moves") {
            const auto move = args.back();
            moves_.push_back(move);
            board_.makeMove(uci::uciToMove(board_, move));
        }
    } else if (args.size() > 1) {
        if (args.at(1) == "startpos") {
            startpos_ = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
            board_ = Board::fromFen(startpos_);
        } else {
            throw std::runtime_error{"Unknown startpos position"};
        }
    }
}

void
Uci::go() {
    callback_(startpos_, moves_);
}

void
Uci::quit() {
    active_ = false;
}

} // namespace segfault
