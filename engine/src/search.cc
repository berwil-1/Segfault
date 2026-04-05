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
Segfault::quiescence(Board & board, int alpha, int beta, uint8_t ply) {
    const auto in_check = board.inCheck();

    if (!in_check) {
        const auto eval = evaluateNetwork(board);

        if (eval >= beta)
            return eval;
        if (eval > alpha)
            alpha = eval;
    }

    Movelist moves;
    if (in_check)
        generateAllMoves(board, moves);
    else
        generateCaptureMoves(board, moves);

    if (in_check && moves.size() == 0)
        return -kMateScore + ply;

    auto best = in_check ? -kMateScore : evaluateNetwork(board);

    for (const auto move : moves) {
        board.makeMove(move);
        const auto score = -quiescence(board, -beta, -alpha, ply + 1);
        board.unmakeMove(move);

        if (score > best)
            best = score;
        if (score > alpha)
            alpha = score;
        if (score >= beta)
            return score;
    }

    return best;
}

int
Segfault::pvs(Board & board, int alpha, int beta, uint8_t depth, uint8_t ply) {
    const bool has_entry = transposition_table_.contains(board.hash());
    const auto scoreToStore = [](int score, uint8_t ply) -> int {
        if (score > kMateScore - 256)
            return score + ply; // convert root-relative → node-relative
        if (score < -kMateScore + 256)
            return score - ply;
        return score;
    };
    const auto scoreFromStore = [](int score, uint8_t ply) -> int {
        if (score > kMateScore - 256)
            return score - ply; // convert node-relative → root-relative
        if (score < -kMateScore + 256)
            return score + ply;
        return score;
    };

    if (has_entry) {
        const auto entry = transposition_table_[board.hash()];
        const auto score = scoreFromStore(entry.eval, ply);

        if (entry.depth >= depth) {
            if (entry.bound == TranspositionTableEntry::EXACT)
                return score;
            if (entry.bound == TranspositionTableEntry::LOWER && score >= beta)
                return score;
            if (entry.bound == TranspositionTableEntry::UPPER && score <= alpha)
                return score;
        }
    }

    if (depth == 0)
        return quiescence(board, alpha, beta, 0);

    Movelist moves;
    generateAllMoves(board, moves);

    // save alpha before update for TT
    const auto pre_alpha = alpha;
    const auto transposition = [this, &pre_alpha,
                                &scoreToStore](const Board & board, const Move move, const int best,
                                               const int alpha, const int beta, const uint8_t depth,
                                               const uint8_t ply) {
        TranspositionTableEntry entry;
        entry.eval = scoreToStore(best, ply);
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

    if (moves.size() == 0) {
        // checkmate
        if (board.inCheck()) {
            transposition(board, Move::NO_MOVE, -kMateScore, alpha, beta, depth, ply);
            return -kMateScore + ply;
        }

        // draw
        return 0;
    }

    // max-heap for move ordering
    auto queue = move_order(board, moves, has_entry);

    auto best_move = queue.top().second;
    board.makeMove(best_move);
    auto best_score = -pvs(board, -beta, -alpha, depth - 1, ply + 1);
    board.unmakeMove(best_move);

    if (best_score > alpha) {
        if (best_score >= beta) {
            transposition(board, best_move, best_score, alpha, beta, depth, ply);
            return best_score;
        }
        alpha = best_score;
    }
    queue.pop();

    while (!queue.empty()) {
        const auto move = queue.top().second;
        board.makeMove(move);
        auto score = -pvs(board, -alpha - 1, -alpha, depth - 1, ply + 1); // alphabeta or zw-search

        if (score > alpha && score < beta) {
            // research with window [alpha;beta]
            score = -pvs(board, -beta, -alpha, depth - 1, ply + 1);

            if (score > alpha)
                alpha = score;
        }
        board.unmakeMove(move);

        if (score > best_score) {
            if (score >= beta) {
                transposition(board, move, score, alpha, beta, depth, ply);
                return score;
            }
            best_move = move;
            best_score = score;
        }
        queue.pop();
    }

    transposition(board, best_move, best_score, alpha, beta, depth, ply);
    return best_score;
}

} // namespace segfault
