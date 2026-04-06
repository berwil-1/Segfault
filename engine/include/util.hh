#pragma once

#include "chess.hh"

namespace segfault {
using namespace chess;

inline void
generateAllMoves(const Board & board, Movelist & list) {
    movegen::legalmoves<movegen::MoveGenType::ALL>(list, board);
}

inline void
generateCaptureMoves(const Board & board, Movelist & list) {
    movegen::legalmoves<movegen::MoveGenType::CAPTURE>(list, board);
}

inline void
generateSpecialMoves(const Board & board, Movelist & list) {
    Movelist moves;
    movegen::legalmoves<movegen::MoveGenType::ALL>(moves, board);

    for (const auto move : moves) {
        const auto is_capture =
            board.at(move.to()) != Piece::NONE && move.typeOf() != Move::CASTLING;
        if (move.typeOf() != Move::NORMAL || is_capture)
            list.add(move);
    }
}

} // namespace segfault
