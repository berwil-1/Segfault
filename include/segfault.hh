#pragma

#include "chess.hh"

#include <cstddef>
#include <cstdint>
#include <unordered_map>
#include <utility>
#include <vector>

namespace segfault {

using namespace chess;

struct TranspositionTableEntry {
    enum Bound : uint16_t { EXACT, LOWER, UPPER };

    Move     move;
    int      eval;
    Bound    bound;
    uint16_t depth;
    uint16_t age;

    /*Zobrist hash;
    int     eval;
    Bound   bound;
    Move    move;
    uint8_t depth;
    uint8_t age;*/

    /*int      score;
    int      alpha;
    int      beta;
    uint16_t depth;
    uint16_t age;*/
};

class Segfault {
public:
    Move
    search(Board & board, std::size_t wtime, std::size_t btime, uint16_t depth);

    int
    quiescence(Board & board, int alpha, int beta, int16_t depth);

    int
    negaAlphaBeta(Board & board, int alpha, int beta, int16_t depth);

private:
    std::unordered_map<uint64_t, TranspositionTableEntry> transposition_table_;
    std::unordered_map<uint64_t, Move>                    iterative_table_;
    std::vector<std::pair<Move, Move>>                    killer_moves_ =
        std::vector<std::pair<Move, Move>>(64, std::pair<Move, Move>{Move::NO_MOVE, Move::NO_MOVE});
    std::unordered_map<uint64_t, std::unordered_map<uint16_t, int>> history_table_;
};

} // namespace segfault
