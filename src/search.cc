#include "search.hh"

#include "eval.hh"
#include "segfault.hh"
#include "util.hh"

#include <chrono>
#include <queue>
#include <random>

namespace segfault {

using namespace chess;

int
Segfault::quiescence(Board & board, int alpha, int beta) {
    const int eval = evaluateNegaAlphaBeta(board);
    int       max = eval;

    /*
     * https://stackoverflow.com/questions/65764015/how-to-implement-transposition-tables-with-alpha-beta-pruning
     *
     * The flags indicate which type of node you have found. If you found a node within your search
     * window (alpha < score < beta) this means you have an EXACT node. Lower bound means that score
     * >= beta, and upper bound that the score <= alpha.
     *
     */

    if (max >= beta) {
        return max;
    }
    if (max > alpha)
        alpha = max;

    Movelist captures;
    generateCaptureMoves(captures, board);

    for (auto capture : captures) {
        board.makeMove(capture);
        int score = -quiescence(board, -beta, -alpha);
        board.unmakeMove(capture);

        if (score >= beta)
            return score;
        if (score > max)
            max = score;
        if (score > alpha)
            alpha = score;
    }

    return max;
}

int
Segfault::negaAlphaBeta(Board & board, int alpha, int beta, int depth) {
    if (depth == 0)
        return quiescence(board, alpha, beta);
    int max = -INT32_MAX;

    Movelist moves;
    generateAllMoves(moves, board);

    for (Move move : moves) {
        board.makeMove(move);
        int score = -negaAlphaBeta(board, -beta, -alpha, depth - 1);
        board.unmakeMove(move);

        if (score > max) {
            max = score;

            if (score > alpha)
                alpha = score;
        }

        if (score >= beta) {
            return max;
        }
    }

    return max;
}

} // namespace segfault
