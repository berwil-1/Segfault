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
Segfault::quiescence(Board & board, int alpha, int beta, int16_t depth) {
    const int eval = evaluateNegaAlphaBeta(board);
    int       max = eval;

    if (depth == 0)
        return eval;

    if (max >= beta) {
        return max;
    }
    if (max > alpha)
        alpha = max;

    Movelist captures;
    generateCaptureMoves(captures, board);

    for (auto capture : captures) {
        board.makeMove(capture);
        int score = -quiescence(board, -beta, -alpha, depth - 1);
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
Segfault::negaAlphaBeta(Board & board, int alpha, int beta, int16_t depth) {
    const auto alpha_before = alpha;

    if (transposition_table_.contains(board.hash()) &&
        transposition_table_.at(board.hash()).depth >= depth) {
        const auto entry = transposition_table_.at(board.hash());

        switch (entry.bound) {
            case TranspositionTableEntry::EXACT: return entry.eval; break;
            case TranspositionTableEntry::LOWER: alpha = std::max(alpha, entry.eval); break;
            case TranspositionTableEntry::UPPER: beta = std::min(beta, entry.eval); break;
        }

        if (alpha >= beta) {
            return entry.eval - depth;
        }
    }

    // TODO: maybe move above tt, not sure?
    switch (board.isGameOver().second) {
        case GameResult::DRAW: return 0 - depth;
        case GameResult::LOSE: return -INT16_MAX - depth;
        case GameResult::WIN: return INT16_MAX - depth;
        case GameResult::NONE:
        default: break;
    }

    if (depth == 0)
        return quiescence(board, alpha, beta, 3) - depth;
    int max = -INT16_MAX;

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
            return max - depth;
        }
    }

    TranspositionTableEntry entry;
    entry.eval = max;
    if (max <= alpha_before) {
        entry.bound = TranspositionTableEntry::UPPER;
    } else if (max >= beta) {
        entry.bound = TranspositionTableEntry::LOWER;
    } else {
        entry.bound = TranspositionTableEntry::EXACT;
    }
    entry.depth = depth;
    transposition_table_.emplace(board.hash(), std::move(entry));

    return max - depth;
}

} // namespace segfault
