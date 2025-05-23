#include "uci.hh"

#include <iostream>
#include <variant>

using namespace segfault;

int
main(int argv, char ** argc) {
    std::string command;
    std::getline(std::cin, command);

    if (command == "uci") {
        Uci uci;
        uci.start();
    }

    return 0;
}
