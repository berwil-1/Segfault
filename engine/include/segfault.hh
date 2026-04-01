#pragma

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

struct TranspositionTableEntry {
    enum Bound : uint8_t { EXACT, LOWER, UPPER };

    Move    move;
    int     eval;
    Bound   bound;
    uint8_t depth;
    uint8_t age;
};

class Segfault {
public:
    Move
    search(Board & board, std::size_t wtime, std::size_t btime);

    int
    quiescence(Board & board, int alpha, int beta);

    int
    pvs(Board & board, int alpha, int beta, uint8_t depth);

private:
    std::unordered_map<uint64_t, TranspositionTableEntry> transposition_table_;
};

} // namespace segfault
