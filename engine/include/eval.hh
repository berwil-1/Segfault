#pragma once

#include "chess.hh"

namespace segfault {

using namespace chess;

int
piece_value_bonus(Board & board, Square square, bool mg);
int
piece_square_table_bonus(Board & board, Square square, Color color, bool mg);
int
imbalance(Board & board, Color color);
bool
isolated(Board & board, Square square, Color color);
bool
connected(Board & board, Square square, Color color);
bool
connected_bonus(Board & board, Square square, Color color);
int
mobility(Board & board, Square square, Color color);
int
mobility_bonus(Board & board, Square square, Color color, bool mg);
int
king_attackers(const Board & board, Square king_square, Color color);
int
king_danger(Board & board, Square king_square, Color color);
int
strength_square(const Board & board, const Color color, const Square square);
int
storm_square(const Board & board, const Color color, const Square square, bool eg = false);
int
shelter_strength(Board & board, Square king_square, Color color, bool eg = false);
int
shelter_storm(const Board & board, Square king_square, Color color, bool eg = false);
int
bishop_pair(const Board & board, const Color color);

// Full evaluation
int
evaluateMiddleGame(Board & board, bool debug = false);
int
evaluateEndGame(Board & board, bool debug = false);
int
phase(Board & board);
int
evaluateStockfish(Board & board, bool debug = false);
int
evaluateSegfault(Board & board);

}; // namespace segfault
