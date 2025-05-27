#pragma once

#include "chess.hh"

namespace segfault {

using namespace chess;

Move
search(Board & board, int depth);

} // namespace segfault
