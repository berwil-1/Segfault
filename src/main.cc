#include "chess.hh"
#include "search.hh"
#include "uci.hh"

#include <iostream>
#include <random>
#include <variant>

using namespace segfault;
using namespace chess;

int
main(int argv, char ** argc) {
    Board board;

    std::string command;
    std::getline(std::cin, command);

    if (command == "uci") {
        Uci uci{board};
        uci.setCallback(
            [&board](const std::string startpos, const std::vector<std::string> & moves) {
                constexpr auto depth = 4;

                const auto move = search(board);
                board.makeMove(move);

                std::cout << "bestmove " << move << std::endl;
            });
        uci.start();
    }

    return 0;
}
