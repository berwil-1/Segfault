#include "chess.hh"
#include "minimal_mlp.hh"

#include <array>
#include <cstdint>
#include <iostream>

using namespace chess;

// Example: mapping piece codes
float
piece_to_value(const char p) {
    switch (p) {
        case 'P': return 1.0f;
        case 'N': return 3.0f;
        case 'B': return 3.25f;
        case 'R': return 5.0f;
        case 'Q': return 9.0f;
        case 'K': return 200.0f;
        case 'p': return -1.0f;
        case 'n': return -3.0f;
        case 'b': return -3.25f;
        case 'r': return -5.0f;
        case 'q': return -9.0f;
        case 'k': return 200.0f;
        default: return 0.0f;
    }
}

// Convert board to input vector
std::array<float, 64>
encode_board(const Board & board) {
    std::array<float, 64> input{};

    constexpr std::array<float, 12> pieces = {1.0f,  3.0f,  3.25f,  5.0f,  9.0f,  200.0f,
                                              -1.0f, -3.0f, -3.25f, -5.0f, -9.0f, -200.0f};

    auto indices = board.occ();

    while (!indices.empty()) {
        const auto index = indices.msb();
        input[index] = pieces[static_cast<int>(board.at(index))];
        indices.clear(index);
    }

    return input;
}

int
main() {
    std::vector<std::array<float, 64>> positions;
    std::vector<float>                 evals; // e.g., 0.25 for +0.25 pawns
    minimal_mlp::MLP                   net; // default: 64 → 32 → 1

    Board board = Board::fromFen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    positions.push_back(encode_board(board));
    evals.push_back(0.07f);

    net.train(positions, evals, 100, 0.01f);
    float eval = net.forward(positions.front());

    std::cout << "Eval = " << eval << "\n";
}
