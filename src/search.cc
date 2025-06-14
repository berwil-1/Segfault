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
    const int eval = evaluateSegfault(board);
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
    int        max = -INT16_MAX;
    Movelist   moves;
    generateAllMoves(moves, board);

    std::vector<std::pair<uint16_t, int>> scores;

    for (const auto move : moves) {
        const auto capture = board.at(move.to()) != Piece::NONE && move.typeOf() != Move::CASTLING;
        int        score = 0;

        // MVV-LVA
        // TODO: add piece/square scores into the mix, this is just to test
        if (capture) {
            std::array<int, 6> values{100, 320, 330, 500, 900};

            const auto attacker = board.at(move.from());
            const auto victim = board.at(move.to());
            const auto victim_value = values[victim.type()];
            const auto attacker_value = values[attacker.type()];
            const auto mvv_lva = victim_value - attacker_value;

            score += mvv_lva;
        }

        // Killer moves
        {
            const auto entry = killer_moves_.at(depth);
            if (entry.first == move)
                score += 200;
            if (entry.second == move)
                score += 150;
        }

        // Rank quiet moves
        if (!capture && history_table_.contains(move.move())) {
            score += history_table_.at(move.move());
        }

        /*if (!capture && history_table_.contains(board.hash()) &&
            history_table_.at(board.hash()).contains(move.move())) {
            score += history_table_.at(board.hash()).at(move.move());
        }*/

        score += move.typeOf() == Move::PROMOTION ? 900 : 0;
        score += move.typeOf() == Move::CASTLING ? 200 : 0;
        // TODO: Checks enemy king, low prio

        scores.emplace_back(move.move(), score);
    }

    // The PV move and TT move should come first
    if (iterative_table_.contains(board.hash())) {
        const auto entry = iterative_table_.at(board.hash());
        for (auto & eval : scores) {
            if (eval.first == entry.move()) {
                eval.second = INT32_MAX;
            }
        }
    }

    if (transposition_table_.contains(board.hash()) &&
        transposition_table_.at(board.hash()).depth >= depth) {
        const auto entry = transposition_table_.at(board.hash());

        switch (entry.bound) {
            case TranspositionTableEntry::EXACT:
                for (auto & eval : scores) {
                    if (eval.first == entry.move.move()) {
                        eval.second = INT32_MAX - 1;
                    }
                }
                return entry.eval;
                break;
            case TranspositionTableEntry::LOWER: alpha = std::max(alpha, entry.eval); break;
            case TranspositionTableEntry::UPPER: beta = std::min(beta, entry.eval); break;
        }

        if (alpha >= beta) {
            return entry.eval - depth;
        }
    }

    switch (board.isGameOver().second) {
        case GameResult::DRAW: return 0 - depth;
        case GameResult::LOSE: return -INT16_MAX - depth;
        case GameResult::WIN: return INT16_MAX - depth;
        case GameResult::NONE:
        default: break;
    }

    if (depth == 0)
        return quiescence(board, alpha, beta, 3) - depth;

    std::sort(scores.begin(), scores.end(),
              [](const auto & a, const auto & b) { return a.second > b.second; });

    Move bestmove;
    for (const auto & eval : scores) {
        board.makeMove(eval.first);
        int score = -negaAlphaBeta(board, -beta, -alpha, depth - 1);
        board.unmakeMove(eval.first);

        if (score > max) {
            max = score;
            bestmove = eval.first;

            if (score > alpha)
                alpha = score;
        }

        // History (gravity)
        auto update = [this, &eval](int bonus) {
            const int clamp = std::clamp(bonus, -128, 128);
            auto &    entry = history_table_[eval.first];

            // Apply exponential decay
            entry -= (entry * std::abs(clamp)) / 128;
            entry += clamp;

            // Optional clamping to avoid runaway scores
            entry = std::clamp(entry, -256, 256);
        };
        const auto bonus = 30 * depth - 25;

        if (score >= beta) {
            if (!board.isCapture(eval.first) && Move{eval.first}.typeOf() != Move::PROMOTION) {
                auto & entry = killer_moves_.at(depth);

                // Killer moves
                if (entry.first != eval.first) {
                    entry.second = entry.first;
                    entry.first = eval.first;
                }

                // History (gravity)
                update(bonus);
            }

            return max - depth;
        } else {
            if (!board.isCapture(eval.first) && Move{eval.first}.typeOf() != Move::PROMOTION) {
                // History (gravity)
                update(-bonus);
            }
        }
    }
    if (bestmove != Move::NO_MOVE) {
        iterative_table_.emplace(board.hash(), bestmove);
    }

    TranspositionTableEntry entry;
    entry.eval = max;
    entry.move = bestmove;
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
