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

static constexpr auto MAX_PLY{128};

struct TranspositionTableEntry {
    enum Bound : uint16_t { EXACT, LOWER, UPPER };

    Move    move; // uint16_t + int16_t
    int     eval;
    Bound   bound;
    uint16_t hash;
    uint16_t age;
    uint8_t depth;
};

static_assert(sizeof(TranspositionTableEntry) == 16);

struct TranspositionTableBlock {
    std::array<TranspositionTableEntry, 4> entries{};
};

static_assert(sizeof(TranspositionTableBlock) == 64);

struct TranspositionTable {
    //std::unordered_map<uint64_t, TranspositionTableBlock> transposition_table_;
    constexpr static std::size_t TT_SIZE{1ULL << 20};
    static_assert((TT_SIZE & (TT_SIZE - 1)) == 0);
    std::vector<TranspositionTableBlock> transposition_table_{TT_SIZE};

    void
    add(const uint64_t index, const auto tt_entry) {
        auto & block = transposition_table_[index & (transposition_table_.size() - 1)].entries;

        for (auto & entry : block) {
            if (entry.hash == 0 || entry.age + 8 <= tt_entry.age) {
                entry = tt_entry;
                return;
            }
        }
    }

    const TranspositionTableEntry* 
    get(const uint64_t index) {
        auto & block = transposition_table_[index & (transposition_table_.size() - 1)].entries;

        for (const auto & entry : block) {
            if (entry.hash == (index & 0xFFFF)) {
                return &entry;
            }
        }
        
        return nullptr;
    }

    bool
    contains(const uint64_t index) {
        auto & block = transposition_table_[index & (transposition_table_.size() - 1)].entries;

        for (const auto & entry : block) {
            if (entry.hash == (index & 0xFFFF)) {
                return true;
            }
        }
        return false;
    }
};

struct PVTable {
    std::array<std::array<Move, MAX_PLY>, MAX_PLY> moves{};
    std::array<int, MAX_PLY>                       length{};
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
    std::array<std::array<Move, 2>, MAX_PLY> killers_{};
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
