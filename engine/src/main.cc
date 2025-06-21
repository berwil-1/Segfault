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
                                            const std::vector<std::string> & moves,
                                            const std::size_t wtime, const std::size_t btime) {
            const auto bestmove = segfault.search(board, wtime, btime);
            board.makeMove(bestmove);

            std::cout << "bestmove " << uci::moveToUci(bestmove) << std::endl;
        });
        uci.start();
    }

    return 0;
}
