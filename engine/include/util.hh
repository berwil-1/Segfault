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

} // namespace segfault
