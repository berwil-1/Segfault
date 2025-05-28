#pragma once

#include "chess.hh"

namespace segfault {

using namespace chess;

int
eval(Board & board);

Move
search(Board & board, int depth);

} // namespace segfault
