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
    if (board.isRepetition(1) || board.isHalfMoveDraw() || board.isInsufficientMaterial())
        return 0;
    const auto in_check = board.inCheck();

    // Retrieve from TT
    // const auto tt_it = transposition_table_.find(board.hash());
    // if (tt_it != transposition_table_.end()) {
    if (transposition_table_.contains(board.hash())) {
        const auto & entry = transposition_table_[board.hash()];
        if (entry.bound == TranspositionTableEntry::EXACT)
            return entry.eval;
        if (entry.bound == TranspositionTableEntry::LOWER && entry.eval >= beta)
            return entry.eval;
        if (entry.bound == TranspositionTableEntry::UPPER && entry.eval <= alpha)
            return entry.eval;
    }

    // Save alpha before update for TT
    const auto pre_alpha = alpha;
    const auto transposition = [this, &pre_alpha](const Board & board, const Move move,
                                                  const int best, const int alpha, const int beta,
                                                  const uint8_t depth, const uint8_t ply) {
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
        transposition_table_[board.hash()] = std::move(entry);
    };

    if (!in_check) {
        const auto eval = evaluateNetwork(board);

        if (eval >= beta) {
            transposition(board, Move::NO_MOVE, eval, alpha, beta, 0, ply);
            return eval;
        }
        if (eval > alpha)
            alpha = eval;
    }

    Movelist moves;
    if (in_check)
        generateAllMoves(board, moves);
    else
        generateSpecialMoves(board, moves);

    if (in_check && moves.empty()) {
        transposition(board, Move::NO_MOVE, -SCORE_MATE + ply, alpha, beta, 0, ply);
        return -SCORE_MATE + ply;
    }

    auto best = in_check ? -SCORE_MATE : evaluateNetwork(board);

    for (const auto move : moves) {
        makeMoveAcc(board, move);
        const auto score = -quiescence(board, -beta, -alpha, ply + 1);
        unmakeMoveAcc(board, move);

        if (score > best)
            best = score;
        if (score > alpha)
            alpha = score;
        if (score >= beta) {
            transposition(board, Move::NO_MOVE, score, alpha, beta, 0, ply);
            return score;
        }
    }

    transposition(board, Move::NO_MOVE, best, alpha, beta, 0, ply);
    return best;
}

int
Segfault::pvs(Board & board, int alpha, int beta, uint8_t depth, uint8_t ply,
              const bool null_move) {
    pv_table_.length[ply] = ply;

    // Draw detection before TT lookup
    if (ply > 0 &&
        (board.isRepetition(1) || board.isHalfMoveDraw() || board.isInsufficientMaterial()))
        return 0;

    // Transposition Table (TT) lookup
    const bool has_entry = transposition_table_.contains(board.hash());
    const auto is_pv_node = (beta - alpha > 1);

    if (has_entry) {
        const auto & entry = transposition_table_[board.hash()];

        if (!is_pv_node && entry.depth >= depth) {
            if (entry.bound == TranspositionTableEntry::EXACT)
                return entry.eval;
            if (entry.bound == TranspositionTableEntry::LOWER && entry.eval >= beta)
                return entry.eval;
            if (entry.bound == TranspositionTableEntry::UPPER && entry.eval <= alpha)
                return entry.eval;
        }
    }

    if (depth == 0)
        return quiescence(board, alpha, beta, ply);

    // Null Move Pruning
    // const auto is_pv_node = (beta - alpha > 1);
    const auto in_check = board.inCheck();

    if (!is_pv_node && !in_check && depth >= 3 && !null_move) {
        // Avoid zugzwang!!
        const auto us = board.sideToMove();
        const auto has_pieces =
            (board.pieces(PieceType::KNIGHT, us) | board.pieces(PieceType::BISHOP, us) |
             board.pieces(PieceType::ROOK, us) | board.pieces(PieceType::QUEEN, us)) != 0;

        if (has_pieces) {
            const auto kNullMoveReduction = 2 + depth / 6;
            board.makeNullMove();
            const auto null_score =
                -pvs(board, -beta, -beta + 1, depth - 1 - kNullMoveReduction, ply + 1, true);
            board.unmakeNullMove();

            if (null_score >= beta)
                return null_score;
        }
    }

    Movelist moves;
    generateAllMoves(board, moves);

    // Save alpha before update for TT
    const auto pre_alpha = alpha;
    const auto transposition = [this, &pre_alpha](const Board & board, const Move move,
                                                  const int best, const int alpha, const int beta,
                                                  const uint8_t depth, const uint8_t ply) {
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
        transposition_table_[board.hash()] = std::move(entry);
    };

    const auto move_order =
        [this](const Board & board, const Movelist & moves, const auto ply,
               const bool has_entry) -> std::priority_queue<std::pair<int, int>> {
        std::priority_queue<std::pair<int, int>> queue;

        // NOTE: do not move out tt[hash], or it may be evaluated...
        const auto entry = has_entry
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
            const auto check_type = board.givesCheck(move);

            // MVV-LVA
            if (is_capture || is_enpassant) {
                constexpr std::array<int, 6> values{100, 300, 325, 500, 900};

                const auto victim_value =
                    values[is_enpassant ? PieceType::PAWN : board.at(move.to()).type()];
                const auto attacker_value = values[board.at(move.from()).type()];
                const auto mvv_lva = victim_value * 10 - attacker_value;

                score += mvv_lva;
            }

            // Killer moves
            if (move == killers_[ply][0]) {
                score += 5000;
            } else if (move == killers_[ply][1]) {
                score += 4500;
            }

            score += move.typeOf() == Move::PROMOTION ? 9000 : 0;
            score += move.typeOf() == Move::CASTLING ? 2000 : 0;
            score += (check_type != CheckType::NO_CHECK) ? 3000 : 0;

            queue.emplace(score, move.move());
        }

        return queue;
    };

    if (moves.size() == 0) {
        // If checkmate, return worst possible score.
        if (board.inCheck()) {
            transposition(board, Move::NO_MOVE, -SCORE_MATE + ply, alpha, beta, depth, ply);
            return -SCORE_MATE + ply;
        }

        // No moves left, must be draw.
        return 0;
    }

    // Max-heap for move ordering based on estimated best moves.
    auto queue = move_order(board, moves, ply, has_entry);

    auto best_move = Move{static_cast<uint16_t>(queue.top().second)};
    makeMoveAcc(board, best_move);
    auto best_score = -pvs(board, -beta, -alpha, depth - 1, ply + 1);
    unmakeMoveAcc(board, best_move);

    if (best_score > alpha) {
        if (best_score >= beta) {
            transposition(board, best_move, best_score, alpha, beta, depth, ply);
            return best_score;
        }
        alpha = best_score;

        // Update PV: this move + child's PV
        pv_table_.moves[ply][ply] = best_move;
        for (auto i = ply + 1; i < pv_table_.length[ply + 1]; ++i)
            pv_table_.moves[ply][i] = pv_table_.moves[ply + 1][i];
        pv_table_.length[ply] = pv_table_.length[ply + 1];
    }
    queue.pop();

    auto move_index{0};
    while (!queue.empty()) {
        const auto move = Move{static_cast<uint16_t>(queue.top().second)};
        // auto score = -pvs(board, -alpha - 1, -alpha, depth - 1, ply + 1);
        const auto in_check = board.inCheck();
        const auto is_capture = board.at(move.to()) != Piece::NONE;
        const auto is_promotion = move.typeOf() == Move::PROMOTION;

        makeMoveAcc(board, move);
        auto reduction = 0;

        // Reduce late quiet moves (LMR)
        if (move_index >= 4 && depth >= 3 && !in_check && !is_capture && !is_promotion) {
            reduction = 1 + move_index / 8;
            reduction = std::min(reduction, depth - 2);
        }
        move_index++;

        auto score = -pvs(board, -alpha - 1, -alpha, depth - 1 - reduction, ply + 1);

        // Re-search at full depth if reduced search beats alpha
        if (reduction > 0 && score > alpha)
            score = -pvs(board, -alpha - 1, -alpha, depth - 1, ply + 1);

        if (score > alpha && score < beta) {
            // Research with window [alpha;beta]
            score = -pvs(board, -beta, -alpha, depth - 1, ply + 1);

            if (score > alpha)
                alpha = score;
        }
        unmakeMoveAcc(board, move);

        if (score > best_score) {
            best_move = move;
            best_score = score;

            if (score > alpha)
                alpha = score;

            // Update PV table
            pv_table_.moves[ply][ply] = move;
            for (auto i = ply + 1; i < pv_table_.length[ply + 1]; ++i)
                pv_table_.moves[ply][i] = pv_table_.moves[ply + 1][i];
            pv_table_.length[ply] = pv_table_.length[ply + 1];

            if (score >= beta) {
                // Store killer move, but only if it's not a capture
                if (board.at(move.to()) == Piece::NONE && move.typeOf() != Move::ENPASSANT) {
                    killers_[ply][1] = killers_[ply][0];
                    killers_[ply][0] = move;
                }
                transposition(board, best_move, best_score, alpha, beta, depth, ply);
                return best_score;
            }
        }
        queue.pop();
    }

    transposition(board, best_move, best_score, alpha, beta, depth, ply);
    return best_score;
}

void
Segfault::makeMoveAcc(Board & board, const Move move) {
    // Push current accumulator
    accumulator_stack_.push_back(accumulator_stack_.back());
    auto & acc = accumulator_stack_.back();

    // Determine feature changes BEFORE makeMove
    const auto piece_from = board.at(move.from());
    const auto captured = board.at(move.to());
    const auto stm = board.sideToMove();

    switch (move.typeOf()) {
        case Move::NORMAL: {
            // Remove moving piece from origin
            acc.sub_feature(weights_, featureIndex(piece_from, move.from()));
            // Add moving piece at destination
            acc.add_feature(weights_, featureIndex(piece_from, move.to()));
            // Remove captured piece if any
            if (captured != Piece::NONE)
                acc.sub_feature(weights_, featureIndex(captured, move.to()));
            break;
        }
        case Move::CASTLING: {
            const auto king = piece_from;
            const auto rook = board.at(move.to()); // rook is at move.to() before castling
            const auto king_side = move.to() > move.from();
            const auto rook_to = Square::castling_rook_square(king_side, stm);
            const auto king_to = Square::castling_king_square(king_side, stm);

            acc.sub_feature(weights_, featureIndex(king, move.from()));
            acc.sub_feature(weights_, featureIndex(rook, move.to()));
            acc.add_feature(weights_, featureIndex(king, king_to));
            acc.add_feature(weights_, featureIndex(rook, rook_to));
            break;
        }
        case Move::PROMOTION: {
            const auto promoted = Piece(move.promotionType(), stm);
            acc.sub_feature(weights_, featureIndex(piece_from, move.from()));
            acc.add_feature(weights_, featureIndex(promoted, move.to()));
            if (captured != Piece::NONE)
                acc.sub_feature(weights_, featureIndex(captured, move.to()));
            break;
        }
        case Move::ENPASSANT: {
            const auto enemy_pawn = Piece(PieceType::PAWN, ~stm);
            const auto ep_capture_sq = static_cast<Square>(move.to().ep_square() ^ 8);
            // Actually: the captured pawn is at (to's file, from's rank)
            // More precisely: square with to's file and from's rank

            acc.sub_feature(weights_, featureIndex(piece_from, move.from()));
            acc.add_feature(weights_, featureIndex(piece_from, move.to()));
            acc.sub_feature(weights_,
                            featureIndex(enemy_pawn, Square(move.to().file(), move.from().rank())));
            break;
        }
    }

    board.makeMove(move);
}

void
Segfault::unmakeMoveAcc(Board & board, const Move move) {
    board.unmakeMove(move);
    accumulator_stack_.pop_back();
}

} // namespace segfault
