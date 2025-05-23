#include "uci.hh"

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

    active = true;
    uci();

    while (active) {
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
    std::cout << command << std::endl;
}

void
Uci::go() {}

void
Uci::quit() {}

} // namespace segfault
