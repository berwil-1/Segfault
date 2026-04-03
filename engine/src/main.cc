#include "chess.hh"
#include "protocol.hh"
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

    // No other protocol types for now...
    const auto protocol = std::make_unique<Uci>(segfault, board);
    protocol->start();

    return 0;
}
