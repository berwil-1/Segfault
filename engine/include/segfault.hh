#pragma

#include "chess.hh"
#include "eval.hh"

#include <cstddef>
#include <cstdint>
#include <unordered_map>
#include <utility>
#include <vector>
#include <array>
#include "torch/torch.h"

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

struct NetImpl : torch::nn::Module {
    torch::nn::Sequential seq;

    explicit NetImpl(int64_t input_dim)
        : seq(torch::nn::Sequential(torch::nn::Linear(input_dim, 256), torch::nn::ReLU(),
                                    torch::nn::Linear(256, 128), torch::nn::ReLU(),
                                    torch::nn::Linear(128, 1))) {
        register_module("seq", seq);
    }

    torch::Tensor
    forward(torch::Tensor x) {
        return seq->forward(x);
    }
};

TORCH_MODULE(Net);

static void
load_module(torch::nn::Module & m, const std::string & path) {
    torch::serialize::InputArchive archive;
    archive.load_from(path);
    m.load(archive);
}

static constexpr int board_size = 320;

std::array<float, board_size>
encode_board(const Board & board);

class Segfault {
public:
    Segfault();

    Move
    search(Board & board, std::size_t wtime, std::size_t btime);

    int
    quiescence(Board & board, int alpha, int beta, int16_t depth);

    int
    negaAlphaBeta(Board & board, int alpha, int beta, int16_t depth);

private:
    std::unordered_map<uint64_t, TranspositionTableEntry> transposition_table_;
    std::unordered_map<uint64_t, Move>                    iterative_table_;
    std::vector<std::pair<Move, Move>>                    killer_moves_ =
        std::vector<std::pair<Move, Move>>(64, std::pair<Move, Move>{Move::NO_MOVE, Move::NO_MOVE});
    // std::unordered_map<uint64_t, std::unordered_map<uint16_t, int>> history_table_;
    std::unordered_map<uint16_t, int> history_table_;
    torch::Device device{torch::kCPU};
    Net model{board_size};
};

} // namespace segfault
