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

        generateAllMoves(moves, board);

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

static int eval_count = 0;

int
quiescence(Board & board, int alpha, int beta) {
    eval_count++;
    const int eval = evaluateNegaMax(board);
    int       max = eval;

    if (max >= beta)
        return max;
    if (max > alpha)
        alpha = max;

    Movelist captures;
    generateCaptureMoves(captures, board);

    for (auto capture : captures) {
        board.makeMove(capture);
        int score = -quiescence(board, -beta, -alpha);
        board.unmakeMove(capture);

        if (score >= beta)
            return score;
        if (score > max)
            max = score;
        if (score > alpha)
            alpha = score;
    }

    return max;
}

int
negaAlphaBeta(Board & board, int alpha, int beta, int depth) {
    if (depth == 0)
        return quiescence(board, alpha, beta);
    int max = -INT32_MAX;

    Movelist moves;
    generateAllMoves(moves, board);

    for (Move move : moves) {
        board.makeMove(move);
        int score = -negaAlphaBeta(board, -beta, -alpha, depth - 1);
        board.unmakeMove(move);

        if (score > max) {
            max = score;

            if (score > alpha)
                alpha = score;
        }

        if (score >= beta)
            return max;
    }

    return max;
}

Move
search(Board & board, int depth) {
    auto     highscore = INT32_MAX;
    Movelist moves;
    Move     bestmove;
    generateAllMoves(moves, board);

    for (Move move : moves) {
        board.makeMove(move);
        const auto score =
            negaAlphaBeta(board, -INT32_MAX, INT32_MAX, depth); // negaMax(board, depth);
        std::cout << move << ": " << score << "\n" << std::flush;

        board.unmakeMove(move);

        if (highscore > score) {
            highscore = score;
            bestmove = move;
        }
    }

    std::cout << "evals: " << eval_count << "\n" << std::flush;
    eval_count = 0;

    return bestmove;
}

} // namespace segfault
