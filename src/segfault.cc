#include "segfault.hh"

#include "search.hh"
#include "util.hh"

namespace segfault {

static_assert(sizeof(TranspositionTableEntry) == 16);
static_assert(alignof(TranspositionTableEntry) == 4);
static_assert(std::is_trivially_copyable_v<TranspositionTableEntry>);

Move
Segfault::search(Board & board, uint16_t depth) {
    auto     highscore = INT32_MAX;
    Movelist moves;
    Move     bestmove;
    generateAllMoves(moves, board);

    for (Move move : moves) {
        board.makeMove(move);
        const auto score = negaAlphaBeta(board, -INT32_MAX, INT32_MAX, depth);
        // std::cout << move << ": " << score << std::endl;

        board.unmakeMove(move);

        if (highscore >= score) {
            highscore = score;
            bestmove = move;
        }
    }

    return bestmove;
}

} // namespace segfault
