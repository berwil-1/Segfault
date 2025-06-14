#pragma once

#include "chess.hh"

namespace segfault {

using namespace chess;

int
evaluateStockfish(Board & board, bool debug = false);

int
evaluateSegfault(Board & board);

}; // namespace segfault
