#include "chess.hh"
#include "uci.hh"

#include <iostream>
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
                Square from{move.substr(0, 2)};
                Square to{move.substr(2, 2)};
                board.makeMove(Move::make(from, to));
            }

            // std::cout << "Fen: " << board.getFen() << std::endl;

            Movelist list;
            gen_all_moves(list, board);
            const auto from = static_cast<std::string>(list.front().from());
            const auto to = static_cast<std::string>(list.front().to());

            std::cout << "bestmove " << from << to << std::endl;
        });
        uci.start();
    }

    return 0;
}
