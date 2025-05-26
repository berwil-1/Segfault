#include "chess.hh"
#include "uci.hh"

#include <iostream>
#include <random>
#include <variant>

using namespace segfault;
using namespace chess;

namespace {
inline void
gen_all_moves(Movelist & list, const Board & board) {
    movegen::legalmoves<movegen::MoveGenType::ALL>(list, board);
}
} // namespace

int
main(int argv, char ** argc) {
    std::string command;
    std::getline(std::cin, command);

    if (command == "uci") {
        Uci uci;
        uci.setCallback([](const std::string startpos, const std::vector<std::string> & moves) {
            Board board = Board::fromFen(startpos);

            for (const auto & move : moves) {
                board.makeMove(uci::uciToMove(board, move));
            }

            Movelist list;
            gen_all_moves(list, board);

            std::random_device                         rd;
            std::mt19937                               gen(rd());
            std::uniform_int_distribution<std::size_t> db(0, list.size() - 1);
            const auto                                 front = list.at(db(gen));

            std::cout << "bestmove " << front << std::endl;
        });
        uci.start();
    }

    return 0;
}
