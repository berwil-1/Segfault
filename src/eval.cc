#include "eval.hh"

#include "util.hh"

#include <array>

namespace segfault {

constexpr std::array<int, 64> mg_pawn_table = {
    0,  0,   0,  0, 0,  0,  0,  0,   50,  50, 50, 50, 50, 50, 50, 50, 10, 10, 20, 30, 30,  20,
    10, 10,  5,  5, 10, 25, 25, 10,  5,   5,  0,  0,  0,  20, 20, 0,  0,  0,  5,  -5, -10, 0,
    0,  -10, -5, 5, 5,  10, 10, -20, -20, 10, 10, 5,  0,  0,  0,  0,  0,  0,  0,  0};

constexpr std::array<int, 64> eg_pawn_table = {
    0,   0,  0,  0, 0,  0,  0,  0,   10,  10, 10, 10, 10,  10, 10, 10,  5,  5, 10, 20, 20, 10,
    5,   5,  0,  0, 0,  20, 20, 0,   0,   0,  5,  -5, -10, 0,  0,  -10, -5, 5, 5,  10, 10, -20,
    -20, 10, 10, 5, 10, 10, 10, -30, -30, 10, 10, 10, 0,   0,  0,  0,   0,  0, 0,  0};

constexpr std::array<int, 64> mg_knight_table = {
    -50, -40, -30, -30, -30, -30, -40, -50, -40, -20, 0,   0,   0,   0,   -20, -40,
    -30, 0,   10,  15,  15,  10,  0,   -30, -30, 5,   15,  20,  20,  15,  5,   -30,
    -30, 0,   15,  20,  20,  15,  0,   -30, -30, 5,   10,  15,  15,  10,  5,   -30,
    -40, -20, 0,   5,   5,   0,   -20, -40, -50, -40, -30, -30, -30, -30, -40, -50};

constexpr std::array<int, 64> eg_knight_table = {
    -40, -30, -20, -20, -20, -20, -30, -40, -30, -10, 0,   5,   5,   0,   -10, -30,
    -20, 5,   10,  15,  15,  10,  5,   -20, -20, 0,   15,  20,  20,  15,  0,   -20,
    -20, 5,   15,  20,  20,  15,  5,   -20, -20, 0,   10,  15,  15,  10,  0,   -20,
    -30, -10, 0,   0,   0,   0,   -10, -30, -40, -30, -20, -20, -20, -20, -30, -40};

constexpr std::array<int, 64> mg_bishop_table = {
    -20, -10, -10, -10, -10, -10, -10, -20, -10, 0,   0,   0,   0,   0,   0,   -10,
    -10, 0,   5,   10,  10,  5,   0,   -10, -10, 5,   5,   10,  10,  5,   5,   -10,
    -10, 0,   10,  10,  10,  10,  0,   -10, -10, 10,  10,  10,  10,  10,  10,  -10,
    -10, 5,   0,   0,   0,   0,   5,   -10, -20, -10, -10, -10, -10, -10, -10, -20};

constexpr std::array<int, 64> eg_bishop_table = {
    -20, -10, -10, -10, -10, -10, -10, -20, -10, 0,   0,   0,   0,   0,   0,   -10,
    -10, 0,   5,   10,  10,  5,   0,   -10, -10, 5,   5,   10,  10,  5,   5,   -10,
    -10, 0,   10,  10,  10,  10,  0,   -10, -10, 10,  10,  10,  10,  10,  10,  -10,
    -10, 5,   0,   0,   0,   0,   5,   -10, -20, -10, -10, -10, -10, -10, -10, -20};

constexpr std::array<int, 64> mg_rook_table = {
    0, 0,  0,  0,  0,  0, 0, 0, 5, 10, 10, 10, 10, 10, 10, 5, -5, 0,  0,  0, 0, 0,
    0, -5, -5, 0,  0,  0, 0, 0, 0, -5, -5, 0,  0,  0,  0,  0, 0,  -5, -5, 0, 0, 0,
    0, 0,  0,  -5, -5, 0, 0, 0, 0, 0,  0,  -5, 0,  0,  0,  5, 5,  0,  0,  0};

constexpr std::array<int, 64> eg_rook_table = {
    0, 0,  0,  5,  5, 0, 0, 0, 5, 10, 10, 10, 10, 10, 10, 5, -5, 0,  0,  0, 0, 0,
    0, -5, -5, 0,  0, 0, 0, 0, 0, -5, -5, 0,  0,  0,  0,  0, 0,  -5, -5, 0, 0, 0,
    0, 0,  0,  -5, 5, 5, 0, 0, 0, 0,  5,  5,  0,  0,  0,  5, 5,  0,  0,  0};

constexpr std::array<int, 64> mg_queen_table = {
    -20, -10, -10, -5, -5, -10, -10, -20, -10, 0,   0,   0,  0,  0,   0,   -10,
    -10, 0,   5,   5,  5,  5,   0,   -10, -5,  0,   5,   5,  5,  5,   0,   -5,
    0,   0,   5,   5,  5,  5,   0,   -5,  -10, 5,   5,   5,  5,  5,   0,   -10,
    -10, 0,   5,   0,  0,  0,   0,   -10, -20, -10, -10, -5, -5, -10, -10, -20};

constexpr std::array<int, 64> eg_queen_table = {
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

// ---------- helpers ---------------------------------------------------------
// MG & EG material values (you can keep your own)
constexpr int MG_VAL[6] = {0, 82, 337, 365, 477, 1025};
constexpr int EG_VAL[6] = {0, 94, 281, 297, 512, 936};

const int FULL_MATERIAL = 8 * 100 + 320 * 2 + 330 * 2 + 500 * 2 + 900;

struct Score {
    int mg = 0, eg = 0;
};

inline Score
operator+(Score a, Score b) {
    return {a.mg + b.mg, a.eg + b.eg};
}

inline Score &
operator+=(Score & a, Score b) {
    a.mg += b.mg;
    a.eg += b.eg;
    return a;
}

// returns opening/ending pair – unchanged
inline Score
piece_score(int sq, PieceType pt) {
    constexpr auto TABLE_SCALE = 2;

    switch (pt) {
        case 0: return {mg_pawn_table[sq] * TABLE_SCALE, eg_pawn_table[sq] * TABLE_SCALE};
        case 1: return {mg_knight_table[sq] * TABLE_SCALE, eg_knight_table[sq] * TABLE_SCALE};
        case 2: return {mg_bishop_table[sq] * TABLE_SCALE, eg_bishop_table[sq] * TABLE_SCALE};
        case 3: return {mg_rook_table[sq] * TABLE_SCALE, eg_rook_table[sq] * TABLE_SCALE};
        case 4: return {mg_queen_table[sq] * TABLE_SCALE, eg_queen_table[sq] * TABLE_SCALE};
        case 5: return {mg_king_table[sq] * TABLE_SCALE, eg_king_table[sq] * TABLE_SCALE};
        default: return {0, 0};
    }
}

int
evaluateNegaAlphaBeta(Board & board) {
    constexpr auto PAWN_VALUE = 100;
    constexpr auto KNIGHT_VALUE = 320;
    constexpr auto BISHOP_VALUE = 330;
    constexpr auto ROOK_VALUE = 500;
    constexpr auto QUEEN_VALUE = 900;

    auto eval = [&](const Color color) -> int {
        Score score{};

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
        auto material = 0;
        material += pawn_count * PAWN_VALUE;
        material += knight_count * KNIGHT_VALUE;
        material += bishop_count * BISHOP_VALUE;
        material += rook_count * ROOK_VALUE;
        material += queen_count * QUEEN_VALUE;
        score.mg += material;

        score.mg += (bishop_count > 1) ? 100 : 0; // bishop pair
        // score.mg -= (rook_count > 1) ? 50 : 0;
        // score.mg -= (knight_count > 1) ? 50 : 0;
        score.mg -= (pawn_count < 1) ? 100 : 0; // pawns for endgame

        constexpr auto table_scale = 2;
        constexpr auto attack_scale = 4;
        const auto     enemy = board.us(~color);
        auto           friends = board.us(color);
        const auto     occ = board.occ();

        while (!friends.empty()) {
            const auto index = friends.msb();

            switch (board.at(index).type().internal()) {
                case PieceType::PAWN: {
                    const auto attacks = attacks::pawn(color, index);
                    score += piece_score(index,
                                         PieceType::PAWN); // mg_pawn_table.at(index) * table_scale;
                    score.mg += attacks.count();
                    score.mg += (enemy & attacks).count() * attack_scale;
                    break;
                }
                case PieceType::KNIGHT: {
                    const auto attacks = attacks::knight(index);
                    score.mg += mg_knight_table.at(index) * table_scale;
                    score.mg += attacks.count();
                    score.mg += (enemy & attacks).count() * attack_scale;
                    break;
                }
                case PieceType::BISHOP: {
                    const auto attacks = attacks::bishop(index, occ);
                    score.mg += mg_bishop_table.at(index) * table_scale;
                    score.mg += attacks.count();
                    score.mg += (enemy & attacks).count() * attack_scale;
                    break;
                }
                case PieceType::ROOK: {
                    const auto attacks = attacks::rook(index, occ);
                    score.mg += mg_rook_table.at(index) * table_scale;
                    score.mg += attacks.count();
                    score.mg += (enemy & attacks).count() * attack_scale;
                    break;
                }
                case PieceType::QUEEN: {
                    const auto attacks = attacks::queen(index, occ);
                    score.mg += mg_queen_table.at(index) * table_scale;
                    score.mg += attacks.count();
                    score.mg += (enemy & attacks).count() * attack_scale;
                    break;
                }
                case PieceType::KING: {
                    const auto attacks = attacks::king(index);
                    score.mg += mg_king_table.at(index) * table_scale;
                    score.mg += attacks.count();
                    score.mg += (enemy & attacks).count() * attack_scale;
                    break;
                }
                case PieceType::NONE:
                default: continue;
            }

            friends.clear(index);
        }

        const int final =
            (score.mg * material + score.eg * (FULL_MATERIAL - material)) / FULL_MATERIAL;
        return final;
    };

    const auto scoreWhite = eval(Color::WHITE);
    const auto scoreBlack = eval(Color::BLACK);

    const auto turn = board.sideToMove() == Color::WHITE;
    const auto scoreTotal = scoreWhite - scoreBlack;
    return turn ? scoreTotal : -scoreTotal;
}

} // namespace segfault
