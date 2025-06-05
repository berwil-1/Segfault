#include "eval.hh"

#include "util.hh"

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

int
evaluateNegaAlphaBeta(Board & board) {
    constexpr auto PAWN_VALUE = 100;
    constexpr auto KNIGHT_VALUE = 320;
    constexpr auto BISHOP_VALUE = 330;
    constexpr auto ROOK_VALUE = 500;
    constexpr auto QUEEN_VALUE = 900;

    auto eval = [&](const Color color) -> int {
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

        constexpr auto table_scale = 1;
        constexpr auto attack_scale = 5;
        const auto     enemy = board.us(~color);
        auto           friends = board.us(color);
        const auto     occ = board.occ();

        while (!friends.empty()) {
            const auto index = friends.msb();

            switch (board.at(index).type().internal()) {
                case PieceType::PAWN: {
                    const auto attacks = attacks::pawn(color, index);
                    score += mg_pawn_table.at(index) * table_scale;
                    score += attacks.count();
                    score += (enemy & attacks).count() * attack_scale;
                    break;
                }
                case PieceType::KNIGHT: {
                    const auto attacks = attacks::knight(index);
                    score += mg_knight_table.at(index) * table_scale;
                    score += attacks.count();
                    score += (enemy & attacks).count() * attack_scale;
                    break;
                }
                case PieceType::BISHOP: {
                    const auto attacks = attacks::bishop(index, occ);
                    score += mg_bishop_table.at(index) * table_scale;
                    score += attacks.count();
                    score += (enemy & attacks).count() * attack_scale;
                    break;
                }
                case PieceType::ROOK: {
                    const auto attacks = attacks::rook(index, occ);
                    score += mg_rook_table.at(index) * table_scale;
                    score += attacks.count();
                    score += (enemy & attacks).count() * attack_scale;
                    break;
                }
                case PieceType::QUEEN: {
                    const auto attacks = attacks::queen(index, occ);
                    score += mg_queen_table.at(index) * table_scale;
                    score += attacks.count();
                    score += (enemy & attacks).count() * attack_scale;
                    break;
                }
                case PieceType::KING: {
                    const auto attacks = attacks::king(index);
                    score += mg_king_table.at(index) * table_scale;
                    score += attacks.count();
                    score += (enemy & attacks).count() * attack_scale;
                    break;
                }
                case PieceType::NONE:
                default: continue;
            }

            friends.clear(index);
        }

        /*for (auto index = 0; index < 64; index++) {
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
        }*/

        /*// Add mobility (number of legal moves)
        Movelist moves;
        bool     nullmove = false;

        if (color != board.sideToMove()) {
            board.makeNullMove();
            nullmove = true;
        }

        generateCaptureMoves(moves, board);

        if (nullmove)
            board.unmakeNullMove();
        score += 5 * moves.size();*/

        return score;
    };

    switch (board.isGameOver().second) {
        case GameResult::DRAW: return 0;
        case GameResult::LOSE: return -INT16_MAX;
        case GameResult::WIN: return INT16_MAX;
        case GameResult::NONE:
        default: break;
    }

    // Draw?
    /*if (board.isRepetition() || board.isHalfMoveDraw() || board.isInsufficientMaterial()) {
        return 0;
    }

    // Game over?
    if (board.inCheck()) {
        Movelist moves;
        generateAllMoves(moves, board);

        if (moves.size() < 1) {
            return -INT16_MAX;
        }
    }*/

    const Color stm = board.sideToMove();
    const int   whiteScore = eval(Color::WHITE);
    const int   blackScore = eval(Color::BLACK);

    const int side = (stm == Color::WHITE ? 1 : -1);
    const int result = side * (whiteScore - blackScore);
    return result;
}

} // namespace segfault
