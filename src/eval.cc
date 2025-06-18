#include "eval.hh"

#include "util.hh"

#include <array>
#include <utility>

namespace segfault {

/**
 * Piece value bonus, material values for middlegame and endgame.
 */
int
piece_value_bonus(Board & board, const Square square, const bool mg) {
    constexpr std::array<int, 64> mg_values = {124, 781, 825, 1276, 2538};
    constexpr std::array<int, 64> eg_values = {206, 854, 915, 1380, 2682};

    const auto index = static_cast<int>(board.at(square).type().internal());
    assert(index < 5 && "piece_value_bonus: outside of values bound");
    return mg ? mg_values[index] : eg_values[index];
}

/**
 * Piece value bonus, material values for middlegame and endgame.
 */
int
piece_square_table_bonus(Board & board, const Square square, const Color color, const bool mg) {
    constexpr auto mg_tables =
        std::array<std::array<std::array<int, 4>, 8>, 5>{{{{{-175, -92, -74, -73},
                                                            {-77, -41, -27, -15},
                                                            {-61, -17, 6, 12},
                                                            {-35, 8, 40, 49},
                                                            {-34, 13, 44, 51},
                                                            {-9, 22, 58, 53},
                                                            {-67, -27, 4, 37},
                                                            {-201, -83, -56, -26}}},
                                                          {{{-53, -5, -8, -23},
                                                            {-15, 8, 19, 4},
                                                            {-7, 21, -5, 17},
                                                            {-5, 11, 25, 39},
                                                            {-12, 29, 22, 31},
                                                            {-16, 6, 1, 11},
                                                            {-17, -14, 5, 0},
                                                            {-48, 1, -14, -23}}},
                                                          {{{-31, -20, -14, -5},
                                                            {-21, -13, -8, 6},
                                                            {-25, -11, -1, 3},
                                                            {-13, -5, -4, -6},
                                                            {-27, -15, -4, 3},
                                                            {-22, -2, 6, 12},
                                                            {-2, 12, 16, 18},
                                                            {-17, -19, -1, 9}}},
                                                          {{{3, -5, -5, 4},
                                                            {-3, 5, 8, 12},
                                                            {-3, 6, 13, 7},
                                                            {4, 5, 9, 8},
                                                            {0, 14, 12, 5},
                                                            {-4, 10, 6, 8},
                                                            {-5, 6, 10, 8},
                                                            {-2, -2, 1, -2}}},
                                                          {{{271, 327, 271, 198},
                                                            {278, 303, 234, 179},
                                                            {195, 258, 169, 120},
                                                            {164, 190, 138, 98},
                                                            {154, 179, 105, 70},
                                                            {123, 145, 81, 31},
                                                            {88, 120, 65, 33},
                                                            {59, 89, 45, -1}}}}};

    constexpr auto eg_tables =
        std::array<std::array<std::array<int, 4>, 8>, 5>{{{{{-96, -65, -49, -21},
                                                            {-67, -54, -18, 8},
                                                            {-40, -27, -8, 29},
                                                            {-35, -2, 13, 28},
                                                            {-45, -16, 9, 39},
                                                            {-51, -44, -16, 17},
                                                            {-69, -50, -51, 12},
                                                            {-100, -88, -56, -17}}},
                                                          {{{-57, -30, -37, -12},
                                                            {-37, -13, -17, 1},
                                                            {-16, -1, -2, 10},
                                                            {-20, -6, 0, 17},
                                                            {-17, -1, -14, 15},
                                                            {-30, 6, 4, 6},
                                                            {-31, -20, -1, 1},
                                                            {-46, -42, -37, -24}}},
                                                          {{{-9, -13, -10, -9},
                                                            {-12, -9, -1, -2},
                                                            {6, -8, -2, -6},
                                                            {-6, 1, -9, 7},
                                                            {-5, 8, 7, -6},
                                                            {6, 1, -7, 10},
                                                            {4, 5, 20, -5},
                                                            {18, 0, 19, 13}}},
                                                          {{{-69, -57, -47, -26},
                                                            {-55, -31, -22, -4},
                                                            {-39, -18, -9, 3},
                                                            {-23, -3, 13, 24},
                                                            {-29, -6, 9, 21},
                                                            {-38, -18, -12, 1},
                                                            {-50, -27, -24, -8},
                                                            {-75, -52, -43, -36}}},
                                                          {{{1, 45, 85, 76},
                                                            {53, 100, 133, 135},
                                                            {88, 130, 169, 175},
                                                            {103, 156, 172, 172},
                                                            {96, 166, 199, 199},
                                                            {92, 172, 184, 191},
                                                            {47, 121, 116, 131},
                                                            {11, 59, 73, 78}}}}};

    constexpr auto mg_tables_pbonus =
        std::array<std::array<int, 8>, 8>{{{0, 0, 0, 0, 0, 0, 0, 0},
                                           {3, 3, 10, 19, 16, 19, 7, -5},
                                           {-9, -15, 11, 15, 32, 22, 5, -22},
                                           {-4, -23, 6, 20, 40, 17, 4, -8},
                                           {13, 0, -13, 1, 11, -2, -13, 5},
                                           {5, -12, -7, 22, -8, -5, -15, -8},
                                           {-7, 7, -3, -13, 5, -16, 10, -8},
                                           {0, 0, 0, 0, 0, 0, 0, 0}}};

    constexpr auto eg_tables_pbonus =
        std::array<std::array<int, 8>, 8>{{{0, 0, 0, 0, 0, 0, 0, 0},
                                           {-10, -6, 10, 0, 14, 7, -5, -19},
                                           {-10, -10, -10, 4, 4, 3, -6, -4},
                                           {6, -2, -8, -4, -13, -12, -10, -9},
                                           {10, 5, 4, -5, -5, -5, 14, 9},
                                           {28, 20, 21, 28, 30, 7, 6, 13},
                                           {0, -11, 12, 21, 25, 19, 4, 7},
                                           {0, 0, 0, 0, 0, 0, 0, 0}}};

    const auto bonus = mg ? mg_tables : eg_tables;
    const auto pbonus = mg ? mg_tables_pbonus : eg_tables_pbonus;
    const auto y = static_cast<int>(square.rank().internal());
    const auto x = static_cast<int>(square.file().internal());

    if (board.at(square).type() == PieceType::PAWN) {
        return pbonus[7 - y][x];
    } else {
        const auto type = static_cast<int>(board.at(square).type().internal()) - 1;
        return bonus[type][7 - y][std::min(x, 7 - x)];
    }
}

int
imbalance(Board & board, const Color color) {
    /*
        if (square == null) return sum(pos, imbalance);
        var qo =
       [[0],[40,38],[32,255,-62],[0,104,4,0],[-26,-2,47,105,-208],[-189,24,117,133,-134,-6]]; var qt
       = [[0],[36,0],[9,63,0],[59,65,42,0],[46,39,24,-24,0],[97,100,-42,137,268,0]]; var j =
       "XPNBRQxpnbrq".indexOf(board(pos, square.x, square.y)); if (j < 0 || j > 5) return 0; var
       bishop = [0, 0], v = 0; for (var x = 0; x < 8; x++) { for (var y = 0; y < 8; y++) { var i =
       "XPNBRQxpnbrq".indexOf(board(pos, x, y)); if (i < 0) continue; if (i == 9) bishop[0]++; if (i
       == 3) bishop[1]++; if (i % 6 > j) continue; if (i > 5) v += qt[j][i-6]; else v += qo[j][i];
            }
        }
        if (bishop[0] > 1) v += qt[j][0];
        if (bishop[1] > 1) v += qo[j][0];
        return v;
    */
    return 0;
}

bool
isolated(Board & board, const Square square, const Color color) {
    const auto file = File{square.index() % 8};

    Bitboard left = file > File::FILE_A ? Bitboard{File{file - 1}} : Bitboard{};
    Bitboard right = file < File::FILE_H ? Bitboard{File{file + 1}} : Bitboard{};

    return (board.pieces(PieceType::PAWN, color) & (left | right)).empty();
}

bool
connected(Board & board, const Square square, const Color color) {
    auto supported = [](Board & board, const Square square, const Color color) {
        const auto backward = color == Color::WHITE ? Direction::SOUTH : Direction::NORTH;
        const auto index = square.index();
        const auto file = File{index % 8};

        Bitboard left = file > File::FILE_A ? Bitboard::fromSquare(index + backward - 1)
                                            : Bitboard{};
        Bitboard right = file < File::FILE_H ? Bitboard::fromSquare(index + backward + 1)
                                             : Bitboard{};
        return (board.pieces(PieceType::PAWN, color) & (left | right)).count();
    };

    auto phalanx = [](Board & board, const Square square, const Color color) {
        const auto index = square.index();
        const auto file = File{index % 8};

        Bitboard left = file > File::FILE_A ? Bitboard::fromSquare(index - 1) : Bitboard{};
        Bitboard right = file < File::FILE_H ? Bitboard::fromSquare(index + 1) : Bitboard{};
        return (board.pieces(PieceType::PAWN, color) & (left | right)).count();
    };

    return supported(board, square, color) || phalanx(board, square, color);
}

bool
connected_bonus(Board & board, const Square square, const Color color) {
    /*
        if (square == null) return sum(pos, connected_bonus);
        if (!connected(pos, square)) return 0;
        var seed = [0, 7, 8, 12, 29, 48, 86];
        var op = opposed(pos, square);
        var ph = phalanx(pos, square);
        var su = supported(pos, square);
        var bl = board(pos, square.x, square.y - 1) == "p" ? 1 : 0;
        var r = rank(pos, square);
        if (r < 2 || r > 7) return 0;
        return seed[r - 1] * (2 + ph - op) + 21 * su;
    */

    return 20; // TODO: fix now
}

int
mobility(Board & board, const Square square, const Color color) {
    auto mobility_area = [](Board & board, const Square square, const Color color) {
        if (board.at(square) == Piece{color, PieceType::KING})
            return 0;
        if (board.at(square) == Piece{color, PieceType::QUEEN})
            return 0;

        // Compute direction of pawn attacks depending on color
        const auto forward = make_direction(Direction::NORTH, color);
        const auto backrank = Rank(7 - (static_cast<int>(color) * 7));

        if (square.file() > File::FILE_A && square.rank() != backrank) {
            Square forward_sq = square + forward;
            if (!forward_sq.is_valid())
                return 0;

            Square target = forward_sq + Direction::WEST;
            if (target.is_valid() && board.at(target) == Piece{~color, PieceType::PAWN})
                return 0;
        }

        if (square.file() < File::FILE_H && square.rank() != backrank) {
            Square forward_sq = square + forward;
            if (!forward_sq.is_valid())
                return 0;

            Square target = forward_sq + Direction::EAST;
            if (target.is_valid() && board.at(target) == Piece{~color, PieceType::PAWN})
                return 0;
        }

        const int r = static_cast<int>(square.rank());
        if (board.at(square) == Piece{color, PieceType::PAWN} &&
            ((color == Color::WHITE && r < 2) || (color == Color::BLACK && r > 5) ||
             board.at(square + forward).type() != PieceType::NONE))
            return 0;

        // TODO: blockers for king

        return 1;
    };

    auto     indices = board.us(color) & board.pieces(PieceType::KNIGHT, PieceType::BISHOP,
                                                      PieceType::ROOK, PieceType::QUEEN);
    auto     score = 0;
    Bitboard area{};

    for (int index = 0; index < 64; index++) {
        if (mobility_area(board, index, color)) {
            area.set(index);
        }
    }

    // std::cerr << "mobility area: \n" << area << std::endl;

    while (!indices.empty()) {
        const auto index = indices.msb();
        const auto piece = board.at(index);

        // TODO: these are not correct
        if (piece.type() == PieceType::KNIGHT) {
            score += (attacks::knight(index) & area).count();
        }
        if (piece.type() == PieceType::BISHOP) {
            score += (attacks::bishop(index, board.occ()) & area).count();
        }
        if (piece.type() == PieceType::ROOK) {
            score += (attacks::rook(index, board.occ()) & area).count();
        }
        if (piece.type() == PieceType::QUEEN) {
            score += (attacks::queen(index, board.occ()) & area).count();
        }

        indices.clear(index);
    }

    return score;
}

int
mobility_bonus(Board & board, const Square square, const Color color, bool mg) {
    const auto mg_tables = std::array<std::vector<int>, 4>{{
        {-62, -53, -12, -4, 3, 13, 22, 28, 33},
        {-48, -20, 16, 26, 38, 51, 55, 63, 63, 68, 81, 81, 91, 98},
        {-60, -20, 2, 3, 3, 11, 22, 31, 40, 40, 41, 48, 57, 57, 62},
        {-30, -12, -8, -9, 20, 23, 23, 35,  38,  53,  64,  65,  65,  66,
         67,  67,  72, 72, 77, 79, 93, 108, 108, 108, 110, 114, 114, 116},
    }};

    const auto eg_tables = std::array<std::vector<int>, 4>{{
        {-81, -56, -31, -16, 5, 11, 17, 20, 25},
        {-59, -23, -3, 13, 24, 42, 54, 57, 65, 73, 78, 86, 88, 97},
        {-78, -17, 23, 39, 70, 99, 103, 121, 134, 139, 158, 164, 168, 169, 172},
        {-48, -30, -7,  19,  40,  55,  59,  75,  78,  96,  96,  100, 121, 127,
         131, 133, 136, 141, 147, 150, 151, 168, 168, 171, 182, 182, 192, 219},
    }};

    const auto table = mg ? mg_tables : eg_tables;
    const auto piece = static_cast<int>(board.at(square).type().internal()) - 1;
    const auto mb =
        std::min(table[piece].size() - 1, static_cast<std::size_t>(mobility(board, square, color)));
    return table[piece][mb];
}

int
king_attackers(const Board & board, const Square king_square, const Color color) {
    auto indices = attacks::king(king_square);
    auto attackers = 0;

    while (!indices.empty()) {
        const auto index = indices.msb();
        attackers += attacks::attackers(board, ~color, index).count();
        indices.clear(index);
    }

    return attackers;
}

int
king_danger(Board & board, const Square king_square, const Color color) {
    constexpr auto values = std::array<int, 5>{0, 81, 52, 44, 10};

    const auto attackers =
        attacks::attackers(board, ~color, king_square); // TODO: may not disregard blocking pieces
    auto indices = attackers & board.pieces(PieceType::PAWN, PieceType::KNIGHT, PieceType::BISHOP,
                                            PieceType::ROOK, PieceType::QUEEN);

    auto count = attackers.count();
    auto king_attacks = king_attackers(board, king_square, color);
    // auto weak = weak_bonus(board, king_square, color);
    // auto unsafe = unsafe_checks(board, king_square, color);
    // auto blockersForKing = blockers_for_king(pos);
    // auto kingFlankAttack = flank_attack(pos);
    // auto kingFlankDefense = flank_defense(pos);
    // auto noQueen = (queen_count(pos) > 0 ? 0 : 1);

    auto weight = 0;
    while (!indices.empty()) {
        const auto index = indices.msb();
        weight += values[static_cast<int>(board.at(index).type().internal())];
        indices.clear(index);
    }

    const auto score = count * weight + 69 * king_attacks;

    if (score > 100) {
        return score;
    } else
        return 0;
}

int
evaluateMiddleGame(Board & board, bool debug = false) {
    auto piece_value_mg = [](Board & board, const Color color) {
        auto indices = board.us(color) & // Only index our side, not enemy side
                       board.pieces(PieceType::PAWN, PieceType::KNIGHT, PieceType::BISHOP,
                                    PieceType::ROOK, PieceType::QUEEN);
        auto score = 0;

        while (!indices.empty()) {
            const auto index = indices.msb();
            score += piece_value_bonus(board, index, true);
            indices.clear(index);
        }

        return score;
    };

    auto piece_square_table_mg = [](Board & board, const Color color, bool debug) {
        auto indices =
            board.us(color) & board.pieces(PieceType::PAWN, PieceType::KNIGHT, PieceType::BISHOP,
                                           PieceType::ROOK, PieceType::QUEEN, PieceType::KING);
        auto score = 0;

        while (!indices.empty()) {
            const auto index = indices.msb();

            if (debug) {
                std::cerr << (color == Color::WHITE ? "  w bonus: " : "  b bonus: ")
                          << piece_square_table_bonus(board, index, color, true) << " (" << index
                          << ")\n";
            }

            score += piece_square_table_bonus(board, index, color, true);
            indices.clear(index);
        }

        return score;
    };

    auto imbalance_total = [](Board & board, const Color color) {
        auto score = 0;

        score += imbalance(board, color); // TODO: this function
        score += (board.pieces(PieceType::BISHOP, color).count() < 2) ? 0 : 1438;

        return (score / 16) << 0;
    };

    auto pawns_mg = [](Board & board, const Color color) {
        auto indices = board.pieces(PieceType::PAWN, color);
        auto score = 0;

        while (!indices.empty()) {
            const auto index = indices.msb();

            // if (doubled_isolated)

            if (isolated(board, index, color))
                score -= 5;

            // if (backward)

            score += connected(board, index, color) ? connected_bonus(board, index, color) : 0;

            // score -= 13 * weak_unopposed_pawn(pos, square);
            // score += [0,-11,-3][blocked(pos, square)];

            indices.clear(index);
        }

        return score;
    };

    auto mobility_mg = [](Board & board, const Color color, bool debug = false) {
        auto indices = board.us(color) & board.pieces(PieceType::KNIGHT, PieceType::BISHOP,
                                                      PieceType::ROOK, PieceType::QUEEN);
        auto score = 0;

        while (!indices.empty()) {
            const auto index = indices.msb();

            if (debug) {
                std::cerr << (color == Color::WHITE ? "  w bonus: " : "  b bonus: ")
                          << mobility_bonus(board, index, color, true) << " (" << index << ")\n";
            }

            score += mobility_bonus(board, index, color, true);

            indices.clear(index);
        }

        return score;
    };

    auto king_mg = [](Board & board, const Color color) {
        auto score = 0;

        const auto king_square = board.kingSq(color);

        auto danger = king_danger(board, king_square, color);
        // score -= shelter_strength(board, color);
        // score += shelter_storm(board, color);
        score += (danger * danger / 4096) << 0;
        // score += 8 * flank_attack(board, color);
        // score += 17 * pawnless_flank(board, color);

        return score;
    };

    constexpr auto color = Color::WHITE;
    auto           score = 0;

    score += piece_value_mg(board, color) - piece_value_mg(board, ~color);
    score +=
        piece_square_table_mg(board, color, debug) - piece_square_table_mg(board, ~color, debug);
    score += imbalance_total(board, color);
    score += pawns_mg(board, color) - pawns_mg(board, ~color);
    // score += pieces_mg(board, color);

    score += mobility_mg(board, color) - mobility_mg(board, ~color);
    score += king_mg(board, color);

    if (debug) {
        std::cerr << "middle game"
                  << "\n";
        std::cerr << " piece_value_mg: "
                  << (piece_value_mg(board, color) - piece_value_mg(board, ~color)) << "\n";
        std::cerr << " piece_square_table_mg: "
                  << (piece_square_table_mg(board, color, debug) -
                      piece_square_table_mg(board, ~color, debug))
                  << "\n";
        std::cerr << " imbalance_total: " << imbalance_total(board, color) << "\n";
        std::cerr << " pawns_mg: " << (pawns_mg(board, color) - pawns_mg(board, ~color)) << "\n";
        std::cerr << " mobility_mg: "
                  << (mobility_mg(board, color, true) - mobility_mg(board, ~color, true)) << "\n";
        std::cerr << " king_mg: " << (king_mg(board, color) - king_mg(board, ~color)) << "\n";
        std::cerr << " score: " << score << "\n";
        std::cerr << std::flush;
    }

    return score;
}

int
evaluateEndGame(Board & board, bool debug = false) {
    auto piece_value_eg = [](Board & board, const Color color) {
        auto indices = board.us(color) & // Only index our side, not enemy side
                       board.pieces(PieceType::PAWN, PieceType::KNIGHT, PieceType::BISHOP,
                                    PieceType::ROOK, PieceType::QUEEN);
        auto score = 0;

        while (!indices.empty()) {
            const auto index = indices.msb();
            score += piece_value_bonus(board, index, false);
            indices.clear(index);
        }

        return score;
    };

    auto piece_square_table_eg = [](Board & board, const Color color, bool debug) {
        auto indices =
            board.us(color) & board.pieces(PieceType::PAWN, PieceType::KNIGHT, PieceType::BISHOP,
                                           PieceType::ROOK, PieceType::QUEEN, PieceType::KING);
        auto score = 0;

        while (!indices.empty()) {
            const auto index = indices.msb();

            if (debug) {
                std::cerr << (color == Color::WHITE ? "  w bonus: " : "  b bonus: ")
                          << piece_square_table_bonus(board, index, color, false) << " (" << index
                          << ")\n";
            }

            score += piece_square_table_bonus(board, index, color, false);
            indices.clear(index);
        }

        return score;
    };

    auto imbalance_total = [](Board & board, const Color color) {
        auto score = 0;

        score += imbalance(board, color); // TODO: this function
        score += (board.pieces(PieceType::BISHOP, color).count() < 2) ? 0 : 1438;

        return (score / 16) << 0;
    };

    auto pawns_eg = [](Board & board, const Color color) {
        auto indices = board.pieces(PieceType::PAWN, color);
        auto score = 0;

        while (!indices.empty()) {
            const auto index = indices.msb();

            // if (doubled_isolated)

            if (isolated(board, index, color))
                score -= 5;

            // if (backward)

            score += connected(board, index, color) ? connected_bonus(board, index, color) : 0;

            // score -= 13 * weak_unopposed_pawn(pos, square);
            // score += [0,-11,-3][blocked(pos, square)];

            indices.clear(index);
        }

        return score;
    };

    auto mobility_eg = [](Board & board, const Color color) {
        auto indices = board.us(color) & board.pieces(PieceType::KNIGHT, PieceType::BISHOP,
                                                      PieceType::ROOK, PieceType::QUEEN);
        auto score = 0;

        while (!indices.empty()) {
            const auto index = indices.msb();
            score += mobility_bonus(board, index, color, false);
            indices.clear(index);
        }

        return score;
    };

    auto king_eg = [](Board & board, const Color color) {
        auto score = 0;

        const auto king_square = board.kingSq(color);

        auto danger = king_danger(board, king_square, color);
        // score -= shelter_strength(board, color);
        // score += shelter_storm(board, color);
        score += (danger * danger / 4096) << 0;
        // score += 8 * flank_attack(board, color);
        // score += 17 * pawnless_flank(board, color);

        return score;
    };

    constexpr auto color = Color::WHITE;
    auto           score = 0;

    score += piece_value_eg(board, color) - piece_value_eg(board, ~color);
    score +=
        piece_square_table_eg(board, color, debug) - piece_square_table_eg(board, ~color, debug);
    score += imbalance_total(board, color);
    score += pawns_eg(board, color) - pawns_eg(board, ~color);
    // score += pieces_eg(board, color);

    score += mobility_eg(board, color) - mobility_eg(board, ~color);
    score += king_eg(board, color);

    if (debug) {
        std::cerr << "endgame"
                  << "\n";
        std::cerr << " piece_value_eg: "
                  << (piece_value_eg(board, color) - piece_value_eg(board, ~color)) << "\n";
        std::cerr << " piece_square_table_eg: "
                  << (piece_square_table_eg(board, color, debug) -
                      piece_square_table_eg(board, ~color, debug))
                  << "\n";
        std::cerr << " imbalance_total: " << imbalance_total(board, color) << "\n";
        std::cerr << " pawns_eg: " << (pawns_eg(board, color) - pawns_eg(board, ~color)) << "\n";
        std::cerr << " mobility_eg: " << (mobility_eg(board, color) - mobility_eg(board, ~color))
                  << "\n";
        std::cerr << " king_eg: " << (king_eg(board, color) - king_eg(board, ~color)) << "\n";
        std::cerr << " score: " << score << "\n";
        std::cerr << std::flush;
    }

    return score;
}

/**
 * Phase. We define phase value for tapered eval based on the amount of non-pawn material on the
 * board.
 */
int
phase(Board & board) {
    constexpr auto midgameLimit = 15258;
    constexpr auto endgameLimit = 3915;

    // Get non-pawn material for all pieces on both sides
    auto non_pawn_material = [](Board & board) {
        auto indices =
            board.pieces(PieceType::KNIGHT, PieceType::BISHOP, PieceType::ROOK, PieceType::QUEEN);
        auto sum = 0;

        while (!indices.empty()) {
            const auto index = indices.msb();
            sum += piece_value_bonus(board, index, true);
            indices.clear(index);
        }

        return sum;
    };

    auto npm = non_pawn_material(board);
    npm = std::clamp(npm, endgameLimit, midgameLimit);
    return (((npm - endgameLimit) * 128) / (midgameLimit - endgameLimit)) << 0;
}

int
evaluateStockfish(Board & board, bool debug) {
    auto mg = evaluateMiddleGame(board, debug);
    auto eg = evaluateEndGame(board, debug);
    auto ph = phase(board);
    auto rule50 = static_cast<int>(board.halfMoveClock());

    // eg = eg * scale_factor(board, eg) / 64;
    auto score = (((mg * ph + ((eg * (128 - ph)) << 0)) / 128) << 0);

    if (debug) {
        std::cerr << "mg: " << mg << "\n";
        std::cerr << "eg: " << eg << "\n";
        std::cerr << "ph: " << ph << "\n";
        std::cerr << "rule50: " << rule50 << "\n";
        std::cerr << "score: " << score << "\n";
    }

    score = (score * (100 - rule50) / 100) << 0;

    if (debug) {
        std::cerr << "score50: " << score << "\n";
    }

    const auto turn = board.sideToMove() == Color::WHITE;
    return turn ? score : -score;
}

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
evaluateSegfault(Board & board) {
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
        score -= (pawn_count < 1) ? 100 : 0;
        score += board.sideToMove() == color ? 28 : 0;

        constexpr auto table_scale = 2;
        constexpr auto attack_scale = 4;
        const auto     enemy = board.us(~color);
        auto           friends = board.us(color);
        const auto     occ = board.occ();

        const auto center = Bitboard::fromSquare(Square::underlying::SQ_E4) |
                            Bitboard::fromSquare(Square::underlying::SQ_D4) |
                            Bitboard::fromSquare(Square::underlying::SQ_E5) |
                            Bitboard::fromSquare(Square::underlying::SQ_D5);

        while (!friends.empty()) {
            const auto index = friends.msb();
            const auto table_index = color == Color::WHITE ? 64 - index : index;

            switch (board.at(index).type().internal()) {
                case PieceType::PAWN: {
                    const auto row = Bitboard{File{index % 8}};
                    const auto pawns = row & board.pieces(PieceType::PAWN, color);
                    const auto attacks = attacks::pawn(color, index);

                    if (pawns.count() > 1)
                        score -= 10;

                    if (attacks & center)
                        score += 20;

                    score += mg_pawn_table[table_index] * table_scale;
                    score += attacks.count();
                    score += (enemy & attacks).count() * attack_scale;
                    break;
                }
                case PieceType::KNIGHT: {
                    const auto attacks = attacks::knight(index);

                    if (attacks & center)
                        score += 40;

                    score += mg_knight_table[table_index] * table_scale;
                    score += attacks.count();
                    score += (enemy & attacks).count() * attack_scale;
                    break;
                }
                case PieceType::BISHOP: {
                    const auto attacks = attacks::bishop(index, occ);

                    if (attacks & center)
                        score += 40;

                    score += mg_bishop_table[table_index] * table_scale;
                    score += attacks.count();
                    score += (enemy & attacks).count() * attack_scale;
                    break;
                }
                case PieceType::ROOK: {
                    const auto attacks = attacks::rook(index, occ);
                    score += mg_rook_table[table_index] * table_scale;
                    score += attacks.count();
                    score += (enemy & attacks).count() * attack_scale;
                    break;
                }
                case PieceType::QUEEN: {
                    const auto attacks = attacks::queen(index, occ);
                    score += mg_queen_table[table_index] * table_scale;
                    score += attacks.count();
                    score += (enemy & attacks).count() * attack_scale;
                    break;
                }
                case PieceType::KING: {
                    const auto attacks = attacks::king(index);
                    score += mg_king_table[table_index] * table_scale;
                    score += attacks.count();
                    score += (enemy & attacks).count() * attack_scale;
                    break;
                }
                case PieceType::NONE:
                default: continue;
            }

            friends.clear(index);
        }

        return score;
    };

    const auto scoreWhite = eval(Color::WHITE);
    const auto scoreBlack = eval(Color::BLACK);

    const auto turn = board.sideToMove() == Color::WHITE;
    const auto scoreTotal = scoreWhite - scoreBlack;
    return turn ? scoreTotal : -scoreTotal;
}

} // namespace segfault
