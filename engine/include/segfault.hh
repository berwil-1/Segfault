#pragma once

#include "chess.hh"
#include "eval.hh"
#include "torch/torch.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <unordered_map>
#include <utility>
#include <vector>

namespace segfault {

using namespace chess;

static constexpr auto kMaxPly{128};

struct TranspositionTableEntry {
    enum Bound : uint8_t { EXACT, LOWER, UPPER };

    Move    move;
    int     eval;
    Bound   bound;
    uint8_t depth;
    uint8_t age;
};

struct PVTable {
    std::array<std::array<Move, kMaxPly>, kMaxPly> moves{};
    std::array<int, kMaxPly>                       length{};
};

class Segfault {
public:
    Move
    search(Board & board, std::size_t wtime, std::size_t btime, std::atomic<bool> & stop);

    int
    quiescence(Board & board, int alpha, int beta, uint8_t ply);

    int
    pvs(Board & board, int alpha, int beta, uint8_t depth, uint8_t ply);

private:
    std::unordered_map<uint64_t, TranspositionTableEntry> transposition_table_;
    std::array<std::array<Move, 2>, kMaxPly>              killers_{};
    PVTable                                               pv_table_;
};

} // namespace segfault
