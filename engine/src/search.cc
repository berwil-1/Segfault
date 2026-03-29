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
    const auto move_order = [](const Board &    board,
                               const Movelist & moves) -> std::priority_queue<std::pair<int, int>> {
        std::priority_queue<std::pair<int, int>> queue;
        for (const auto move : moves) {
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

            /*// Killer moves
            {
                const auto entry = killer_moves_.at(depth);
                if (entry.first == move) {
                    score += 200;
                }
                if (entry.second == move) {
                    score += 150;
                }
            }*/

            score += move.typeOf() == Move::PROMOTION ? 9000 : 0;
            score += move.typeOf() == Move::CASTLING ? 2000 : 0;

            // TODO: Checks enemy king, low prio

            queue.emplace(score, move.move());
        }

        return queue;
    };

    if (depth == 0)
        return quiescence(board, alpha, beta);

    Movelist moves;
    generateAllMoves(board, moves);

    if (moves.size() == 0)
        return evaluateNetwork(board);

    // max-heap for move ordering
    auto queue = move_order(board, moves);

    board.makeMove(queue.top().second);
    auto best = -pvs(board, -beta, -alpha, depth - 1);
    board.unmakeMove(queue.top().second);

    if (best > alpha) {
        if (best >= beta)
            return best;
        alpha = best;
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

        if (score > best) {
            if (score >= beta)
                return score;
            best = score;
        }
        queue.pop();
    }

    return best;
}

} // namespace segfault
