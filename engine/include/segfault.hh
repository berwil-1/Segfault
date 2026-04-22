#pragma once

#include "chess.hh"
#include "eval.hh"
#include "matmul.hh"
// #include "torch/torch.h"

#include <array>
#include <atomic>
#include <chrono>
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

struct TranspositionTable {
    std::unordered_map<uint64_t, TranspositionTableEntry> transposition_table_;

    TranspositionTableEntry &
    operator[](uint64_t index) {
        return transposition_table_[index];
    }

    bool
    contains(uint64_t index) const {
        return transposition_table_.contains(index);
    }
};

struct PVTable {
    std::array<std::array<Move, kMaxPly>, kMaxPly> moves{};
    std::array<int, kMaxPly>                       length{};
};

class Segfault {
public:
    Segfault();

    int
    evaluateNetwork(const Board & board);

    Move
    search(Board & board, std::size_t wtime, std::size_t btime, std::size_t winc, std::size_t binc,
           std::atomic<bool> & stop);

    Move
    search(Board & board, uint8_t depth, std::atomic<bool> & stop);

    int
    quiescence(Board & board, int alpha, int beta, uint8_t ply);

    int
    pvs(Board & board, int alpha, int beta, uint8_t depth, uint8_t ply,
        const bool null_move = false);

    void
    makeMoveAcc(Board & board, const Move move);

    void
    unmakeMoveAcc(Board & board, const Move move);

private:
    TranspositionTable                       transposition_table_;
    std::array<std::array<Move, 2>, kMaxPly> killers_{};
    std::array<std::array<int, 64>, 12>      history_{};
    PVTable                                  pv_table_;
    NetworkWeights                           weights_;
    std::vector<Accumulator>                 accumulator_stack_;

    std::chrono::time_point<std::chrono::system_clock> deadline_{
        std::chrono::system_clock::time_point::max()};
    bool        search_aborted_{false};
    std::size_t nodes_{0};
};

} // namespace segfault
