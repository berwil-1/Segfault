#include "uci.hh"

#include <algorithm>
#include <iostream>
#include <sstream>

namespace {
auto
split(std::string line, char delimiter) {
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

void
Uci::start() {
    std::string line;

    active_ = true;
    uci();

    while (active_) {
        std::getline(std::cin, line);
        const auto args = split(line, ' ');

        // Check for exact match
        auto it = commands.find(args.front());

        if (it != commands.end()) {
            it->second(line);
        } else if (line.starts_with("position")) {
            position(line);
        } else {
            throw std::runtime_error{"Unknown command"};
        }
    }
}

std::vector<std::string>
Uci::getMoves() {
    return std::vector<std::string>();
}

std::string
Uci::getStartFen() {
    return std::string();
}

void
Uci::setCallback(Callback func) {
    callback_ = func;
}

void
Uci::uci() {
    std::cout << "id name segfault\n"
              << "id author William Bergh\n"
              << "uciok\n"
              << std::flush;
}

void
Uci::isready() {
    std::cout << "readyok\n" << std::flush;
}

void
Uci::ucinewgame() {}

void
Uci::position(const std::string & command) {
    const auto args = split(command, ' ');

    if (args.size() > 1) {
        if (args.at(1) == "startpos") {
            startpos_ = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
        } else {
            throw std::runtime_error{"Unknown position startpos"};
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
