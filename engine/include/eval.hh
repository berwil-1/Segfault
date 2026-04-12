#pragma once

#include "chess.hh"
#include "torch/torch.h"

namespace segfault {

using namespace chess;

int
piece_value_bonus(const Board & board, const Square square, bool mg);
int
piece_square_table_bonus(const Board & board, const Square square, const chess::Color color,
                         bool mg);
int
imbalance(const Board & board, const chess::Color color);
bool
isolated(const Board & board, const Square square, const chess::Color color);
bool
connected(const Board & board, const Square square, const chess::Color color);
bool
connected_bonus(const Board & board, const Square square, const chess::Color color);
int
mobility(const Board & board, const Square square, const chess::Color color);
int
mobility_bonus(const Board & board, const Square square, const chess::Color color, bool mg);
int
king_attackers(const Board & board, const Square king_square, const chess::Color color);
int
king_danger(const Board & board, const Square king_square, const chess::Color color);
int
strength_square(const Board & board, const chess::Color color, const Square square);
int
storm_square(const Board & board, const chess::Color color, const Square square, bool eg = false);
int
shelter_strength(const Board & board, const Square king_square, const chess::Color color,
                 bool eg = false);
int
shelter_storm(const Board & board, const Square king_square, const chess::Color color,
              bool eg = false);
int
bishop_pair(const Board & board, const chess::Color color);

// Full evaluation
int
evaluateMiddleGame(const Board & board, bool debug = false);
int
evaluateEndGame(const Board & board, bool debug = false);
int
phase(const Board & board);
int
evaluateStockfish(const Board & board, bool debug = false);
int
evaluateSegfault(const Board & board);

// constexpr int BOARD_SIZE = 264;
constexpr int BOARD_SIZE_NNUE = 768;

std::array<float, BOARD_SIZE_NNUE>
encode_board(const Board & board);

// int
// evaluateNetwork(const Board & board);

}; // namespace segfault
