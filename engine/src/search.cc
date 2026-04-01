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
    generateCaptureMoves(board, captures);

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
}

int
Segfault::pvs(Board & board, int alpha, int beta, uint8_t depth) {
    const bool has_entry = transposition_table_.contains(board.hash());

    if (has_entry) {
        const auto entry = transposition_table_[board.hash()];

        if (entry.depth >= depth) {
            if (entry.bound == TranspositionTableEntry::EXACT) {
                return entry.eval;
            }

            if (entry.bound == TranspositionTableEntry::LOWER && entry.eval >= beta) {
                return entry.eval;
            }

            if (entry.bound == TranspositionTableEntry::UPPER && entry.eval <= alpha) {
                return entry.eval;
            }
        }
    }

    if (depth == 0)
        return quiescence(board, alpha, beta);

    Movelist moves;
    generateAllMoves(board, moves);

    if (moves.size() == 0)
        return evaluateNetwork(board);

    // save alpha before update for TT
    const auto pre_alpha = alpha;
    const auto transposition = [this, &pre_alpha](const Board & board, const Move move,
                                                  const int best, const int alpha, const int beta,
                                                  const uint8_t depth) {
        TranspositionTableEntry entry;
        entry.eval = best;
        entry.move = move;
        if (best <= pre_alpha) {
            entry.bound = TranspositionTableEntry::UPPER;
        } else if (best >= beta) {
            entry.bound = TranspositionTableEntry::LOWER;
        } else {
            entry.bound = TranspositionTableEntry::EXACT;
        }
        entry.depth = depth;
        transposition_table_.emplace(board.hash(), std::move(entry));
    };

    const auto move_order =
        [this](const Board & board, const Movelist & moves,
               const bool has_entry) -> std::priority_queue<std::pair<int, int>> {
        std::priority_queue<std::pair<int, int>> queue;
        const auto                               entry = has_entry
                                                             ? std::make_optional<Move>(transposition_table_[board.hash()].move)
                                                             : std::nullopt;

        for (const auto move : moves) {
            if (move == entry) {
                queue.emplace(INT_MAX, move.move());
                continue;
            }

            int        score = 0;
            const auto is_capture =
                board.at(move.to()) != Piece::NONE && move.typeOf() != Move::CASTLING;
            const auto is_enpassant = move.typeOf() == Move::ENPASSANT;

            // MVV-LVA
            if (is_capture || is_enpassant) {
                constexpr std::array<int, 6> values{100, 300, 325, 500, 900};

                const auto victim_value =
                    values[is_enpassant ? PieceType::PAWN : board.at(move.to()).type()];
                const auto attacker_value = values[board.at(move.from()).type()];
                const auto mvv_lva = victim_value * 10 - attacker_value;

                score += mvv_lva;
            }

            // TODO: killer moves

            score += move.typeOf() == Move::PROMOTION ? 9000 : 0;
            score += move.typeOf() == Move::CASTLING ? 2000 : 0;
            // TODO: checks enemy king, high prio?

            queue.emplace(score, move.move());
        }

        return queue;
    };
    // max-heap for move ordering
    auto queue = move_order(board, moves, has_entry);

    auto best_move = queue.top().second;
    board.makeMove(best_move);
    auto best_score = -pvs(board, -beta, -alpha, depth - 1);
    board.unmakeMove(best_move);

    if (best_score > alpha) {
        if (best_score >= beta) {
            transposition(board, best_move, best_score, alpha, beta, depth);
            return best_score;
        }
        alpha = best_score;
    }
    queue.pop();

    while (!queue.empty()) {
        const auto move = queue.top().second;
        board.makeMove(move);
        auto score = -pvs(board, -alpha - 1, -alpha, depth - 1); // alphabeta or zw-search

        if (score > alpha && score < beta) {
            // research with window [alpha;beta]
            score = -pvs(board, -beta, -alpha, depth - 1);

            if (score > alpha)
                alpha = score;
        }
        board.unmakeMove(move);

        if (score > best_score) {
            if (score >= beta) {
                transposition(board, move, score, alpha, beta, depth);
                return score;
            }
            best_move = move;
            best_score = score;
        }
        queue.pop();
    }

    transposition(board, best_move, best_score, alpha, beta, depth);
    return best_score;
}

} // namespace segfault
