#pragma once

#include "chess.hh"

namespace segfault {
using namespace chess;

inline void
generateAllMoves(Movelist & list, const Board & board) {
    movegen::legalmoves<movegen::MoveGenType::ALL>(list, board);
}

inline void
generateCaptureMoves(Movelist & list, const Board & board) {
    movegen::legalmoves<movegen::MoveGenType::CAPTURE>(list, board);
}

} // namespace segfault
