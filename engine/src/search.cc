#include "search.hh"

#include "eval.hh"
#include "segfault.hh"
#include "util.hh"

#include <chrono>
#include <iterator>
#include <queue>
#include <random>

namespace segfault {

using namespace chess;

int
Segfault::quiescence(Board & board, int alpha, int beta) {
    const int eval = evaluateNetwork(board);

    int best = eval;

    if (best >= beta)
        return best;
    if (best > alpha)
        return best;

    Movelist captures;
    generateCaptureMoves(captures, board);

    for (auto capture : captures) {
        board.makeMove(capture);
        int score = -quiescence(board, -beta, -alpha);
        board.unmakeMove(capture);

        if (score >= beta)
            return score;
        if (score > best)
            best = score;
        if (score > alpha)
            alpha = score;
    }

    return best;

    /*// const int eval = evaluateSegfault(board);
    const int eval = evaluateNetwork(board);
    int       max = eval;

    if (depth == 0) {
        return eval;
    }

    if (max >= beta) {
        return max;
    }
    if (max > alpha) {
        alpha = max;
    }

    Movelist captures;
    generateCaptureMoves(captures, board);

    for (auto capture : captures) {
        board.makeMove(capture);
        int score = -quiescence(board, -beta, -alpha, depth - 1);
        board.unmakeMove(capture);

        if (score >= beta) {
            return score;
        }
        if (score > max) {
            max = score;
        }
        if (score > alpha) {
            alpha = score;
        }
    }

    return max;*/
}

int
Segfault::alphaBeta(Board & board, int alpha, int beta, uint8_t depth) {
    int best = -INT32_MAX;

    Movelist moves;
    generateAllMoves(moves, board);
    for (const auto move : moves) {
        board.makeMove(move);
        // NOTE: negated and beta and alpha are flipped
        int score = (depth - 1 == 0) ? -quiescence(board, -beta, -alpha)
                                     : -alphaBeta(board, -beta, -alpha, depth - 1);
        board.unmakeMove(move);

        if (score > best) {
            best = score;

            if (score > alpha)
                alpha = score; // alpha = max in minimax
        }
        if (score >= beta)
            return best; // fail soft beta-cutoff
    }
    return best;
}

int
Segfault::pvs(Board & board, int alpha, int beta, uint8_t depth) {
    if (depth == 0)
        return quiescence(board, alpha, beta);

    Movelist moves;
    generateAllMoves(moves, board);

    if (moves.size() == 0)
        return evaluateNetwork(board);

    board.makeMove(*moves.begin());
    auto best = -pvs(board, -beta, -alpha, depth - 1);
    board.unmakeMove(*moves.begin());

    if (best > alpha) {
        if (best >= beta)
            return best;
        alpha = best;
    }
    moves.pop_front();

    for (const auto move : moves) {
        board.makeMove(move);
        auto score = -pvs(board, -alpha - 1, -alpha, depth - 1); // alphabeta or zw-search

        if (score > alpha && score < beta) {
            // research with window [alpha;beta]
            score = -pvs(board, -beta, -alpha, depth - 1);

            if (score > alpha)
                alpha = score;
        }
        board.unmakeMove(move);

        if (score > best) {
            if (score >= beta)
                return score;
            best = score;
        }
    }

    return best;
}

} // namespace segfault
