#include "eval.hh"

#include <array>

namespace segfault {

constexpr std::array<int, 64> mg_pawn_table = {
    0,  0,   0,  0, 0,  0,  0,  0,   50,  50, 50, 50, 50, 50, 50, 50, 10, 10, 20, 30, 30,  20,
    10, 10,  5,  5, 10, 25, 25, 10,  5,   5,  0,  0,  0,  20, 20, 0,  0,  0,  5,  -5, -10, 0,
    0,  -10, -5, 5, 5,  10, 10, -20, -20, 10, 10, 5,  0,  0,  0,  0,  0,  0,  0,  0};

constexpr std::array<int, 64> mg_knight_table = {
    -50, -40, -30, -30, -30, -30, -40, -50, -40, -20, 0,   0,   0,   0,   -20, -40,
    -30, 0,   10,  15,  15,  10,  0,   -30, -30, 5,   15,  20,  20,  15,  5,   -30,
    -30, 0,   15,  20,  20,  15,  0,   -30, -30, 5,   10,  15,  15,  10,  5,   -30,
    -40, -20, 0,   5,   5,   0,   -20, -40, -50, -40, -30, -30, -30, -30, -40, -50};

constexpr std::array<int, 64> mg_bishop_table = {
    -20, -10, -10, -10, -10, -10, -10, -20, -10, 0,   0,   0,   0,   0,   0,   -10,
    -10, 0,   5,   10,  10,  5,   0,   -10, -10, 5,   5,   10,  10,  5,   5,   -10,
    -10, 0,   10,  10,  10,  10,  0,   -10, -10, 10,  10,  10,  10,  10,  10,  -10,
    -10, 5,   0,   0,   0,   0,   5,   -10, -20, -10, -10, -10, -10, -10, -10, -20};

constexpr std::array<int, 64> mg_rook_table = {
    0, 0,  0,  0,  0,  0, 0, 0, 5, 10, 10, 10, 10, 10, 10, 5, -5, 0,  0,  0, 0, 0,
    0, -5, -5, 0,  0,  0, 0, 0, 0, -5, -5, 0,  0,  0,  0,  0, 0,  -5, -5, 0, 0, 0,
    0, 0,  0,  -5, -5, 0, 0, 0, 0, 0,  0,  -5, 0,  0,  0,  5, 5,  0,  0,  0};

constexpr std::array<int, 64> mg_queen_table = {
    -20, -10, -10, -5, -5, -10, -10, -20, -10, 0,   0,   0,  0,  0,   0,   -10,
    -10, 0,   5,   5,  5,  5,   0,   -10, -5,  0,   5,   5,  5,  5,   0,   -5,
    0,   0,   5,   5,  5,  5,   0,   -5,  -10, 5,   5,   5,  5,  5,   0,   -10,
    -10, 0,   5,   0,  0,  0,   0,   -10, -20, -10, -10, -5, -5, -10, -10, -20};

constexpr std::array<int, 64> mg_king_table = {
    -30, -40, -40, -50, -50, -40, -40, -30, -30, -40, -40, -50, -50, -40, -40, -30,
    -30, -40, -40, -50, -50, -40, -40, -30, -30, -40, -40, -50, -50, -40, -40, -30,
    -20, -30, -30, -40, -40, -30, -30, -20, -10, -20, -20, -20, -20, -20, -20, -10,
    20,  20,  0,   0,   0,   0,   20,  20,  20,  30,  10,  0,   0,   10,  30,  20};

constexpr std::array<int, 64> eg_king_table = {
    -50, -40, -30, -20, -20, -30, -40, -50, -30, -20, -10, 0,   0,   -10, -20, -30,
    -30, -10, 20,  30,  30,  20,  -10, -30, -30, -10, 30,  40,  40,  30,  -10, -30,
    -30, -10, 30,  40,  40,  30,  -10, -30, -30, -10, 20,  30,  30,  20,  -10, -30,
    -30, -30, 0,   0,   0,   0,   -30, -30, -50, -30, -30, -30, -30, -30, -30, -50};

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
        score += std::min(2 * moves.size(), 50);

        return score;
    };

    int whiteScore = eval(Color::WHITE);
    int blackScore = eval(Color::BLACK);

    int result = (stm == Color::WHITE ? 1 : -1) * (whiteScore - blackScore);
    return board.inCheck() ? -INT32_MAX : result;
}

int
evaluateNegaAlphaBeta(Board & board) {
    constexpr auto PAWN_VALUE = 100;
    constexpr auto KNIGHT_VALUE = 320;
    constexpr auto BISHOP_VALUE = 330;
    constexpr auto ROOK_VALUE = 500;
    constexpr auto QUEEN_VALUE = 900;

    auto eval = [&](Color color) -> int {
        int score = 0;

        const auto pawn = board.pieces(PieceType::PAWN, color);
        const auto knight = board.pieces(PieceType::KNIGHT, color);
        const auto bishop = board.pieces(PieceType::BISHOP, color);
        const auto rook = board.pieces(PieceType::ROOK, color);
        const auto queen = board.pieces(PieceType::QUEEN, color);

        const auto pawn_count = pawn.count();
        const auto knight_count = knight.count();
        const auto bishop_count = bishop.count();
        const auto rook_count = rook.count();
        const auto queen_count = queen.count();

        // Not pretty but probably fastest way to do this
        score += pawn_count * PAWN_VALUE;
        score += knight_count * KNIGHT_VALUE;
        score += bishop_count * BISHOP_VALUE;
        score += rook_count * ROOK_VALUE;
        score += queen_count * QUEEN_VALUE;

        score += (bishop_count > 1) ? 100 : 0;
        score -= (rook_count > 1) ? 50 : 0;
        score -= (knight_count > 1) ? 50 : 0;
        score -= (pawn_count < 1) ? 100 : 0;

        const auto     occ = board.occ();
        constexpr auto table_scale = 1;

        for (auto index = 0; index < 64; index++) {
            // HACK: fix this later
            if (board.at(index).color() != color) {
                continue;
            }

            switch (board.at(index).type().internal()) {
                case PieceType::PAWN:
                    score += mg_pawn_table.at(index) * table_scale;
                    score += attacks::pawn(color, index).count();
                    break;
                case PieceType::KNIGHT:
                    score += mg_knight_table.at(index) * table_scale;
                    score += attacks::knight(index).count();
                    break;
                case PieceType::BISHOP:
                    score += mg_bishop_table.at(index) * table_scale;
                    score += attacks::bishop(index, occ).count();
                    break;
                case PieceType::ROOK:
                    score += mg_rook_table.at(index) * table_scale;
                    score += attacks::rook(index, occ).count();
                    break;
                case PieceType::QUEEN:
                    score += mg_queen_table.at(index) * table_scale;
                    score += attacks::queen(index, occ).count();
                    break;
                case PieceType::KING:
                    score += mg_king_table.at(index) * table_scale;
                    score += attacks::king(index).count();
                    break;
                case PieceType::NONE:
                default: continue;
            }
        }

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
        score += 5 * moves.size();

        /*Bitboard occ_us = board.us(c);
        Bitboard occ_opp = board.us(~c);
        Bitboard occ_all = occ_us | occ_opp;

        Bitboard opp_empty = ~occ_us;
        Bitboard seen = seenSquares<color>(board, opp_empty);*/

        return score;
    };

    if (board.isRepetition() || board.isHalfMoveDraw() || board.isInsufficientMaterial()) {
        return 0;
    }

    /*if (board.isGameOver().second == GameResult::DRAW) {
        return 0;
    }*/

    if (board.inCheck()) {
        Movelist moves;
        generateAllMoves(moves, board);

        if (moves.size() < 1) {
            return -INT32_MAX;
        }
    }

    const Color stm = board.sideToMove();
    const int   whiteScore = eval(Color::WHITE);
    const int   blackScore = eval(Color::BLACK);

    const int side = (stm == Color::WHITE ? 1 : -1);
    const int result = side * (whiteScore - blackScore);
    return result;

    // TODO: in check eval is dumb af, it should be checkmate cause elsewise it just checks and
    // thinks it's won the game...
    // int result = (stm == Color::WHITE ? 1 : -1) * (whiteScore - blackScore);
    // return board.inCheck() ? -INT32_MAX : result;
}

int
evaluate(Board & board) {
    return evaluateNegaAlphaBeta(board);
}

} // namespace segfault
