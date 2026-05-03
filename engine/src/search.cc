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

inline int
score_to_tt(const int score, const uint8_t ply) {
    if (score > SCORE_MATE - 100)
        return score + ply;
    if (score < -SCORE_MATE + 100)
        return score - ply;
    return score;
}

inline int
score_from_tt(const int score, const uint8_t ply) {
    if (score > SCORE_MATE - 100)
        return score - ply;
    if (score < -SCORE_MATE + 100)
        return score + ply;
    return score;
}

int
Segfault::quiescence(Board & board, SearchContext & ctx, int alpha, int beta, uint8_t ply) {
    if (board.isRepetition(1) || board.isHalfMoveDraw() || board.isInsufficientMaterial())
        return -33;
    const auto * entry = transposition_table_.get(board.hash());
    const auto   in_check = board.inCheck();

    // Retrieve from TT
    if (entry != nullptr) {
        const auto tt_score = score_from_tt(entry->eval, ply);
        if (entry->bound == TranspositionTableEntry::EXACT)
            return tt_score;
        if (entry->bound == TranspositionTableEntry::LOWER && tt_score >= beta)
            return tt_score;
        if (entry->bound == TranspositionTableEntry::UPPER && tt_score <= alpha)
            return tt_score;
    }

    // Save alpha before update for TT
    const auto pre_alpha = alpha;
    const auto transposition = [this, &pre_alpha](const Board & board, const Move move,
                                                  const int best, const int alpha, const int beta,
                                                  const uint8_t depth, const uint8_t ply) {
        TranspositionTableEntry entry;
        entry.eval = score_to_tt(best, ply);
        entry.move = move;

        if (best <= pre_alpha) {
            entry.bound = TranspositionTableEntry::UPPER;
        } else if (best >= beta) {
            entry.bound = TranspositionTableEntry::LOWER;
        } else {
            entry.bound = TranspositionTableEntry::EXACT;
        }

        entry.hash = static_cast<uint16_t>(board.hash() >> 48);
        entry.age = board.fullMoveNumber();
        entry.depth = depth;
        transposition_table_.add(board.hash(), entry);
    };
    auto best = in_check ? -SCORE_MATE : evaluateNetwork(board, ctx);

    if (!in_check) {
        if (best >= beta) {
            transposition(board, Move::NO_MOVE, best, alpha, beta, 0, ply);
            return best;
        }

        // Delta prune
        constexpr auto MARGIN = 2000;
        if (best + MARGIN < alpha) {
            transposition(board, Move::NO_MOVE, best, alpha, beta, 0, ply);
            return best;
        }

        if (best > alpha)
            alpha = best;
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

    for (const auto move : moves) {
        makeMoveAcc(board, ctx, move);
        const auto score = -quiescence(board, ctx, -beta, -alpha, ply + 1);
        unmakeMoveAcc(board, ctx, move);

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
Segfault::pvs(Board & board, SearchContext & ctx, int alpha, int beta, uint8_t depth, uint8_t ply,
              const bool null_move) {
    ctx.pv_table.length[ply] = ply;

    // Draw detection before TT lookup
    if (ply > 0 &&
        (board.isRepetition(1) || board.isHalfMoveDraw() || board.isInsufficientMaterial()))
        return -33;

    if (ctx.nodes++ % 4096 == 0 && std::chrono::system_clock::now() > deadline_)
        search_aborted_ = true;
    if (search_aborted_)
        return 0;

    // Transposition Table (TT) lookup
    const auto * entry = transposition_table_.get(board.hash());
    const auto   is_pv_node = (beta - alpha > 1);

    if (entry != nullptr) {
        const auto tt_score = score_from_tt(entry->eval, ply);

        if (!is_pv_node && entry->depth >= depth) {
            if (entry->bound == TranspositionTableEntry::EXACT)
                return tt_score;
            if (entry->bound == TranspositionTableEntry::LOWER && tt_score >= beta)
                return tt_score;
            if (entry->bound == TranspositionTableEntry::UPPER && tt_score <= alpha)
                return tt_score;
        }
    }

    if (depth == 0)
        return quiescence(board, ctx, alpha, beta, ply);

    // Null Move Pruning
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
                -pvs(board, ctx, -beta, -beta + 1, depth - 1 - kNullMoveReduction, ply + 1, true);
            board.unmakeNullMove();

            if (null_score >= beta)
                return null_score;
        }
    }

    const auto static_eval = evaluateNetwork(board, ctx);
    const auto can_futility_prune = !is_pv_node && !in_check && depth <= 3;

    Movelist moves;
    generateAllMoves(board, moves);

    // Save alpha before update for TT
    const auto pre_alpha = alpha;
    const auto transposition = [this, &pre_alpha](const Board & board, const Move move,
                                                  const int best, const int alpha, const int beta,
                                                  const uint8_t depth, const uint8_t ply) {
        TranspositionTableEntry entry;
        entry.eval = score_to_tt(best, ply);
        entry.move = move;

        if (best <= pre_alpha) {
            entry.bound = TranspositionTableEntry::UPPER;
        } else if (best >= beta) {
            entry.bound = TranspositionTableEntry::LOWER;
        } else {
            entry.bound = TranspositionTableEntry::EXACT;
        }

        entry.hash = static_cast<uint16_t>(board.hash() >> 48);
        entry.age = board.fullMoveNumber();
        entry.depth = depth;
        transposition_table_.add(board.hash(), entry);
    };

    const auto move_order =
        [this, &ctx](const Board & board, const Movelist & moves, const auto ply,
                     const auto * entry_ptr) -> std::priority_queue<std::pair<int, int>> {
        const auto                               entry = entry_ptr == nullptr ? std::nullopt
                                                                              : std::make_optional<Move>(entry_ptr->move);
        std::priority_queue<std::pair<int, int>> queue;

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
                constexpr std::array<int, 6> VALUES{100, 300, 325, 500, 900};

                const auto victim_value =
                    VALUES[is_enpassant ? PieceType::PAWN : board.at(move.to()).type()];
                const auto attacker_value = VALUES[board.at(move.from()).type()];
                const auto mvv_lva = victim_value * 10 - attacker_value;

                score += mvv_lva;
            }

            if (is_capture && ctx.last_move_to != Square::NO_SQ && move.to() == ctx.last_move_to) {
                score += 4000;
            }

            // Killer moves
            if (move == ctx.killers[ply][0]) {
                score += 5000;
            } else if (move == ctx.killers[ply][1]) {
                score += 4500;
            }

            if (ply > 0 && ctx.last_move_to != Square::NO_SQ) {
                const auto piece = board.at(ctx.last_move_to);
                if (piece != Piece::NONE) {
                    if (move == ctx.countermoves[static_cast<int>(piece)][ctx.last_move_to.index()])
                        score += 4000;
                }
            }

            // History heuristics
            if (!is_capture && !is_enpassant) {
                score +=
                    ctx.history[static_cast<int>(board.at(move.from()))][move.to().index()] / 100;
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
    auto queue = move_order(board, moves, ply, entry);

    auto best_move = Move{static_cast<uint16_t>(queue.top().second)};
    transposition_table_.prefetch(board.zobristAfter<false>(best_move));
    makeMoveAcc(board, ctx, best_move);
    auto extension = board.inCheck() ? 1 : 0;
    auto best_score = -pvs(board, ctx, -beta, -alpha, depth - 1 + extension, ply + 1);
    unmakeMoveAcc(board, ctx, best_move);
    if (search_aborted_) {
        return 0;
    }

    if (best_score > alpha) {
        if (best_score >= beta) {
            transposition(board, best_move, best_score, alpha, beta, depth, ply);
            return best_score;
        }
        alpha = best_score;

        // Update PV: this move + child's PV
        ctx.pv_table.moves[ply][ply] = best_move;
        for (auto i = ply + 1; i < ctx.pv_table.length[ply + 1]; i++)
            ctx.pv_table.moves[ply][i] = ctx.pv_table.moves[ply + 1][i];
        ctx.pv_table.length[ply] = ctx.pv_table.length[ply + 1];
    }
    queue.pop();

    auto move_index{0};
    while (!queue.empty()) {
        const auto move = Move{static_cast<uint16_t>(queue.top().second)};
        transposition_table_.prefetch(board.zobristAfter<false>(move));
        queue.pop();
        const auto is_capture = board.at(move.to()) != Piece::NONE;
        const auto is_promotion = move.typeOf() == Move::PROMOTION;

        // Futility prune
        if (can_futility_prune && move_index > 0 && !is_capture && !is_promotion) {
            constexpr std::array<int, 4> MARGINS{0, 2000, 4000, 6000};
            if (static_eval + MARGINS[depth] <= alpha) {
                move_index++;
                continue;
            }
        }

        const auto in_check = board.inCheck();
        const auto gives_check = board.givesCheck(move) != CheckType::NO_CHECK;

        // Late move prune
        if (!is_pv_node && !in_check && depth <= 2 && !is_capture && !is_promotion &&
            !gives_check) {
            constexpr std::array<int, 3> lmp_thresholds{0, 6, 12};
            if (move_index >= lmp_thresholds[depth]) {
                move_index++;
                continue;
            }
        }

        makeMoveAcc(board, ctx, move);
        auto extension = board.inCheck() ? 1 : 0;
        auto reduction = 0;

        // Reduce late quiet moves (LMR)
        if (move_index >= 3 && depth >= 3 && !in_check && !is_capture && !is_promotion &&
            !gives_check) {
            reduction = 1 + move_index / 8;
            reduction = std::min(reduction, depth - 2);
        }
        move_index++;

        auto score =
            -pvs(board, ctx, -alpha - 1, -alpha, depth - 1 - reduction + extension, ply + 1);
        if (search_aborted_) {
            unmakeMoveAcc(board, ctx, move);
            return 0;
        }

        // Re-search at full depth if reduced search beats alpha
        if (reduction > 0 && score > alpha) {
            score = -pvs(board, ctx, -alpha - 1, -alpha, depth - 1 + extension, ply + 1);
            if (search_aborted_) {
                unmakeMoveAcc(board, ctx, move);
                return 0;
            }
        }

        if (score > alpha && score < beta) {
            // Research with window [alpha;beta]
            score = -pvs(board, ctx, -beta, -alpha, depth - 1 + extension, ply + 1);
            if (search_aborted_) {
                unmakeMoveAcc(board, ctx, move);
                return 0;
            }

            if (score > alpha)
                alpha = score;
        }
        unmakeMoveAcc(board, ctx, move);

        if (score > best_score) {
            best_move = move;
            best_score = score;

            if (score > alpha)
                alpha = score;

            // Update PV table
            ctx.pv_table.moves[ply][ply] = move;
            for (auto i = ply + 1; i < ctx.pv_table.length[ply + 1]; i++)
                ctx.pv_table.moves[ply][i] = ctx.pv_table.moves[ply + 1][i];
            ctx.pv_table.length[ply] = ctx.pv_table.length[ply + 1];

            // Beta cutoff?
            if (score >= beta) {
                // Store killer move, but only if it's not a capture
                if (board.at(move.to()) == Piece::NONE && move.typeOf() != Move::ENPASSANT) {
                    ctx.killers[ply][1] = ctx.killers[ply][0];
                    ctx.killers[ply][0] = move;
                    ctx.history[static_cast<int>(board.at(move.from()))][move.to().index()] +=
                        depth * depth;
                }

                if (ply > 0 && ctx.last_move_to != Square::NO_SQ) {
                    const auto piece = board.at(ctx.last_move_to);
                    if (piece != Piece::NONE) {
                        ctx.countermoves[static_cast<int>(piece)][ctx.last_move_to.index()] = move;
                    }
                }

                transposition(board, best_move, best_score, alpha, beta, depth, ply);
                return best_score;
            }
        }
    }

    transposition(board, best_move, best_score, alpha, beta, depth, ply);
    return best_score;
}

void
Segfault::makeMoveAcc(Board & board, SearchContext & ctx, const Move move) {
    // Push current accumulator
    auto copy = ctx.accumulator_stack.back();
    ctx.accumulator_stack.push_back(std::move(copy));
    ctx.last_move_stack.push_back(ctx.last_move_to);
    auto & acc = ctx.accumulator_stack.back();

    // Determine feature changes BEFORE makeMove
    const auto piece_from = board.at(move.from());
    const auto captured = board.at(move.to());
    const auto stm = board.sideToMove();

    switch (move.typeOf()) {
        case Move::NORMAL: {
            acc.sub_feature(network_weights_, featureIndex(piece_from, move.from()));
            acc.add_feature(network_weights_, featureIndex(piece_from, move.to()));
            if (captured != Piece::NONE)
                acc.sub_feature(network_weights_, featureIndex(captured, move.to()));
            break;
        }
        case Move::CASTLING: {
            const auto king = piece_from;
            const auto rook = board.at(move.to());
            const auto king_side = move.to() > move.from();
            const auto rook_to = Square::castling_rook_square(king_side, stm);
            const auto king_to = Square::castling_king_square(king_side, stm);

            acc.sub_feature(network_weights_, featureIndex(king, move.from()));
            acc.sub_feature(network_weights_, featureIndex(rook, move.to()));
            acc.add_feature(network_weights_, featureIndex(king, king_to));
            acc.add_feature(network_weights_, featureIndex(rook, rook_to));
            break;
        }
        case Move::PROMOTION: {
            const auto promoted = Piece(move.promotionType(), stm);
            acc.sub_feature(network_weights_, featureIndex(piece_from, move.from()));
            acc.add_feature(network_weights_, featureIndex(promoted, move.to()));
            if (captured != Piece::NONE)
                acc.sub_feature(network_weights_, featureIndex(captured, move.to()));
            break;
        }
        case Move::ENPASSANT: {
            const auto enemy_pawn = Piece(PieceType::PAWN, ~stm);
            acc.sub_feature(network_weights_, featureIndex(piece_from, move.from()));
            acc.add_feature(network_weights_, featureIndex(piece_from, move.to()));
            acc.sub_feature(network_weights_,
                            featureIndex(enemy_pawn, Square(move.to().file(), move.from().rank())));
            break;
        }
    }

    ctx.last_move_to = move.to();
    board.makeMove(move);
}

void
Segfault::unmakeMoveAcc(Board & board, SearchContext & ctx, const Move move) {
    board.unmakeMove(move);
    ctx.accumulator_stack.pop_back();
    ctx.last_move_to = ctx.last_move_stack.back();
    ctx.last_move_stack.pop_back();
}

} // namespace segfault
