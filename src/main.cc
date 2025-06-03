#include "chess.hh"
#include "segfault.hh"
#include "uci.hh"

#include <iostream>
#include <random>
#include <variant>

using namespace segfault;
using namespace chess;

int
main(int argv, char ** argc) {
    Segfault segfault;
    Board    board;

    std::string command;
    std::getline(std::cin, command);

    if (command == "uci") {
        Uci uci{board};
        uci.setCallback([&segfault, &board](const std::string                startpos,
                                            const std::vector<std::string> & moves) {
            constexpr auto depth = 3;
            const auto     move = segfault.search(board, depth);
            board.makeMove(move);

            std::cout << "bestmove " << move << std::endl;
        });
        uci.start();
    }

    return 0;
}
