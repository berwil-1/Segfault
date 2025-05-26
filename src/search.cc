#include "search.hh"

#include <chrono>
#include <queue>
#include <random>

namespace segfault {

using namespace chess;

inline void
generateAllMoves(Movelist & list, const Board & board) {
    movegen::legalmoves<movegen::MoveGenType::ALL>(list, board);
}

inline void
generateCaptureMoves(Movelist & list, const Board & board) {
    movegen::legalmoves<movegen::MoveGenType::CAPTURE>(list, board);
}

int
evaluateNegaMax(Board & board) {
    constexpr int PAWN_VALUE = 100;
    constexpr int KNIGHT_VALUE = 320;
    constexpr int BISHOP_VALUE = 330;
    constexpr int ROOK_VALUE = 500;
    constexpr int QUEEN_VALUE = 900;

    const Color stm = board.sideToMove();

    auto eval = [&](Color color) -> int {
        int score = 0;
        score += board.pieces(PieceType::PAWN, color).count() * PAWN_VALUE;
        score += board.pieces(PieceType::KNIGHT, color).count() * KNIGHT_VALUE;
        score += board.pieces(PieceType::BISHOP, color).count() * BISHOP_VALUE;
        score += board.pieces(PieceType::ROOK, color).count() * ROOK_VALUE;
        score += board.pieces(PieceType::QUEEN, color).count() * QUEEN_VALUE;

        // Add mobility (number of legal moves)
        Movelist moves;
        bool     nullmove = false;

        if (color != board.sideToMove()) {
            board.makeNullMove();
            nullmove = true;
        }

        generateCaptureMoves(moves, board);

        if (nullmove)
            board.unmakeNullMove();
        score += 2 * moves.size(); // mobility weight

        return score;
    };

    int whiteScore = eval(Color::WHITE);
    int blackScore = eval(Color::BLACK);

    int result = (stm == Color::WHITE ? 1 : -1) * (whiteScore - blackScore);
    return board.inCheck() ? -INT32_MAX : result;
}

Move
randMove(const Board & board) {
    Movelist list;
    generateAllMoves(list, board);

    std::random_device                         random;
    std::mt19937                               twister(random());
    std::uniform_int_distribution<std::size_t> uniform(0, list.size() - 1);

    return list.at(uniform(twister));
}

int
negaMax(Board & board, int depth) {
    if (depth == 0)
        return evaluateNegaMax(board);
    int max = -INT32_MAX;

    Movelist moves;
    generateAllMoves(moves, board);

    for (Move move : moves) {
        board.makeMove(move);
        int score = negaMax(board, depth - 1);
        board.unmakeMove(move);

        if (score > max)
            max = score;
    }
    return max;
}

Move
search(Board & board) {
    Movelist moves;
    Move     bestmove;
    generateAllMoves(moves, board);

    auto highscore = -INT32_MAX;
    auto depth = moves.size() < 40 ? 3 : 2;

    for (Move move : moves) {
        board.makeMove(move);
        const auto score = negaMax(board, depth);
        board.unmakeMove(move);

        if (highscore < score) {
            highscore = score;
            bestmove = move;
        }
    }

    return bestmove;
}

} // namespace segfault
