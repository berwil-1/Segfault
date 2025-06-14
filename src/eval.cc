#include "eval.hh"

#include "util.hh"

#include <array>

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
    constexpr auto mg_tables = std::array<std::array<int, 64>, 6>{{
        // pawn
        {0,  0,   0,  0, 0,  0,  0,  0,   50,  50, 50, 50, 50, 50, 50, 50, 10, 10, 20, 30, 30,  20,
         10, 10,  5,  5, 10, 25, 25, 10,  5,   5,  0,  0,  0,  20, 20, 0,  0,  0,  5,  -5, -10, 0,
         0,  -10, -5, 5, 5,  10, 10, -20, -20, 10, 10, 5,  0,  0,  0,  0,  0,  0,  0,  0},
        // knight
        {-50, -40, -30, -30, -30, -30, -40, -50, -40, -20, 0,   0,   0,   0,   -20, -40,
         -30, 0,   10,  15,  15,  10,  0,   -30, -30, 5,   15,  20,  20,  15,  5,   -30,
         -30, 0,   15,  20,  20,  15,  0,   -30, -30, 5,   10,  15,  15,  10,  5,   -30,
         -40, -20, 0,   5,   5,   0,   -20, -40, -50, -40, -30, -30, -30, -30, -40, -50},
        // bishop
        {-20, -10, -10, -10, -10, -10, -10, -20, -10, 0,   0,   0,   0,   0,   0,   -10,
         -10, 0,   5,   10,  10,  5,   0,   -10, -10, 5,   5,   10,  10,  5,   5,   -10,
         -10, 0,   10,  10,  10,  10,  0,   -10, -10, 10,  10,  10,  10,  10,  10,  -10,
         -10, 5,   0,   0,   0,   0,   5,   -10, -20, -10, -10, -10, -10, -10, -10, -20},
        // rook
        {0, 0,  0,  0,  0,  0, 0, 0, 5, 10, 10, 10, 10, 10, 10, 5, -5, 0,  0,  0, 0, 0,
         0, -5, -5, 0,  0,  0, 0, 0, 0, -5, -5, 0,  0,  0,  0,  0, 0,  -5, -5, 0, 0, 0,
         0, 0,  0,  -5, -5, 0, 0, 0, 0, 0,  0,  -5, 0,  0,  0,  5, 5,  0,  0,  0},
        // queen
        {-20, -10, -10, -5, -5, -10, -10, -20, -10, 0,   0,   0,  0,  0,   0,   -10,
         -10, 0,   5,   5,  5,  5,   0,   -10, -5,  0,   5,   5,  5,  5,   0,   -5,
         0,   0,   5,   5,  5,  5,   0,   -5,  -10, 5,   5,   5,  5,  5,   0,   -10,
         -10, 0,   5,   0,  0,  0,   0,   -10, -20, -10, -10, -5, -5, -10, -10, -20},
        // king
        {-30, -40, -40, -50, -50, -40, -40, -30, -30, -40, -40, -50, -50, -40, -40, -30,
         -30, -40, -40, -50, -50, -40, -40, -30, -30, -40, -40, -50, -50, -40, -40, -30,
         -20, -30, -30, -40, -40, -30, -30, -20, -10, -20, -20, -20, -20, -20, -20, -10,
         20,  20,  0,   0,   0,   0,   20,  20,  20,  30,  10,  0,   0,   10,  30,  20},
    }};

    constexpr auto eg_tables = std::array<std::array<int, 64>, 6>{{
        // pawn
        {0,   0,  0,  0, 0,  0,  0,  0,   10,  10, 10, 10, 10,  10, 10, 10,  5,  5, 10, 20, 20, 10,
         5,   5,  0,  0, 0,  20, 20, 0,   0,   0,  5,  -5, -10, 0,  0,  -10, -5, 5, 5,  10, 10, -20,
         -20, 10, 10, 5, 10, 10, 10, -30, -30, 10, 10, 10, 0,   0,  0,  0,   0,  0, 0,  0},
        // knight
        {-40, -30, -20, -20, -20, -20, -30, -40, -30, -10, 0,   5,   5,   0,   -10, -30,
         -20, 5,   10,  15,  15,  10,  5,   -20, -20, 0,   15,  20,  20,  15,  0,   -20,
         -20, 5,   15,  20,  20,  15,  5,   -20, -20, 0,   10,  15,  15,  10,  0,   -20,
         -30, -10, 0,   0,   0,   0,   -10, -30, -40, -30, -20, -20, -20, -20, -30, -40},

        // bishop
        {-20, -10, -10, -10, -10, -10, -10, -20, -10, 0,   0,   0,   0,   0,   0,   -10,
         -10, 0,   5,   10,  10,  5,   0,   -10, -10, 5,   5,   10,  10,  5,   5,   -10,
         -10, 0,   10,  10,  10,  10,  0,   -10, -10, 10,  10,  10,  10,  10,  10,  -10,
         -10, 5,   0,   0,   0,   0,   5,   -10, -20, -10, -10, -10, -10, -10, -10, -20},

        // rook
        {0, 0,  0,  5,  5, 0, 0, 0, 5, 10, 10, 10, 10, 10, 10, 5, -5, 0,  0,  0, 0, 0,
         0, -5, -5, 0,  0, 0, 0, 0, 0, -5, -5, 0,  0,  0,  0,  0, 0,  -5, -5, 0, 0, 0,
         0, 0,  0,  -5, 5, 5, 0, 0, 0, 0,  5,  5,  0,  0,  0,  5, 5,  0,  0,  0},

        // queen
        {-20, -10, -10, -5, -5, -10, -10, -20, -10, 0,   0,   0,  0,  0,   0,   -10,
         -10, 0,   5,   5,  5,  5,   0,   -10, -5,  0,   5,   5,  5,  5,   0,   -5,
         0,   0,   5,   5,  5,  5,   0,   -5,  -10, 5,   5,   5,  5,  5,   0,   -10,
         -10, 0,   5,   0,  0,  0,   0,   -10, -20, -10, -10, -5, -5, -10, -10, -20},

        // king
        {-50, -40, -30, -20, -20, -30, -40, -50, -30, -20, -10, 0,   0,   -10, -20, -30,
         -30, -10, 20,  30,  30,  20,  -10, -30, -30, -10, 30,  40,  40,  30,  -10, -30,
         -30, -10, 30,  40,  40,  30,  -10, -30, -30, -10, 20,  30,  30,  20,  -10, -30,
         -30, -30, 0,   0,   0,   0,   -30, -30, -50, -30, -30, -30, -30, -30, -30, -50},
    }};

    const auto index = color ? 64 - square.index() : square.index();
    const auto table = mg ? mg_tables : eg_tables;
    const auto piece = static_cast<int>(board.at(square).type().internal());

    return table[piece][index];
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

    return 20;
}

int
mobility(Board & board, const Square square, const Color color) {
    auto mobility_area = [](Board & board, const Square square, const Color color) {
        if (board.at(square).type() == PieceType::KING)
            return 0;
        if (board.at(square).type() == PieceType::QUEEN)
            return 0;

        const int file = square.file();
        const int rank = square.rank();

        // Compute direction of pawn attacks depending on color
        const auto forward =
            static_cast<int>((color == Color::WHITE) ? Direction::NORTH : Direction::SOUTH);

        if (square.file() > File::FILE_A &&
            board.at(Square{File{square.index() + static_cast<int>(Direction::WEST)},
                            Rank{rank + forward}}) == Piece{~color, PieceType::PAWN})
            return 0;
        if (square.file() < File::FILE_H &&
            board.at(Square{File{square.index() + static_cast<int>(Direction::EAST)},
                            Rank{rank + forward}}) == Piece{~color, PieceType::PAWN})
            return 0;

        return 1;
    };

    auto indices =
        board.us(color) &
        board.pieces(PieceType::KNIGHT, PieceType::BISHOP, PieceType::ROOK, PieceType::QUEEN);
    auto     score = 0;
    Bitboard area{};

    for (int i = 0; i < 64; i++) {
        area.set(mobility_area(board, i, true));
    }

    while (!indices.empty()) {
        const auto index = indices.msb();
        // score += mobility_area(board, index, true);

        // if (!mobility_area(board, square, color))
        //     continue;

        const auto piece = board.at(index);

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

    /*if (square == Square::NO_SQ)
        return 0;

    const auto type = board.at(square).type();
    if (type == PieceType::KING || type == PieceType::QUEEN)
        return 0;

    const int file = square.file();
    const int rank = square.rank();

    // Compute direction of pawn attacks depending on color
    const auto forward =
        static_cast<int>((color == Color::WHITE) ? Direction::NORTH : Direction::SOUTH);

    // Opponent color
    const auto enemy_pawn = Piece{PieceType::PAWN, ~color};

    // Check for enemy pawn threats (diagonals)
    if (file > 0 && board.at(Square{File{file - 1}, Rank{rank + forward}}) == enemy_pawn)
        return 0;
    if (file < 7 && board.at(Square{File{file + 1}, Rank{rank + forward}}) == enemy_pawn)
        return 0;

    // Pawn restriction for own pawns
    if (type == PieceType::PAWN && color == board.sideToMove()) {
        if ((color == Color::WHITE && rank < 3) || (color == Color::BLACK && rank > 4))
            return 0;

        const auto behind =
            static_cast<int>((color == Color::WHITE) ? Direction::SOUTH : Direction::NORTH);
        if (board.at(Square{File{file}, Rank{rank + behind}}) != Piece::NONE)
            return 0;
    }

    // Dummy flipped square if needed
    // Square flipped = Square{File{file}, Rank{7 - rank}};

    // Example blocker check (replace with actual logic)
    // if (blockers_for_king(~color, flipped))
    //     return 0;

    return 1;*/
}

int
mobility_bonus(Board & board, const Square square, const Color color, bool mg) {
    const auto mg_tables = std::array<std::array<int, 28>, 4>{{
        {-62, -53, -12, -4, 3, 13, 22, 28, 33},
        {-48, -20, 16, 26, 38, 51, 55, 63, 63, 68, 81, 81, 91, 98},
        {-60, -20, 2, 3, 3, 11, 22, 31, 40, 40, 41, 48, 57, 57, 62},
        {-30, -12, -8, -9, 20, 23, 23, 35,  38,  53,  64,  65,  65,  66,
         67,  67,  72, 72, 77, 79, 93, 108, 108, 108, 110, 114, 114, 116},
    }};

    const auto eg_tables = std::array<std::array<int, 28>, 4>{{
        {-81, -56, -31, -16, 5, 11, 17, 20, 25},
        {-59, -23, -3, 13, 24, 42, 54, 57, 65, 73, 78, 86, 88, 97},
        {-78, -17, 23, 39, 70, 99, 103, 121, 134, 139, 158, 164, 168, 169, 172},
        {-48, -30, -7,  19,  40,  55,  59,  75,  78,  96,  96,  100, 121, 127,
         131, 133, 136, 141, 147, 150, 151, 168, 168, 171, 182, 182, 192, 219},
    }};

    const auto table = mg ? mg_tables : eg_tables;
    const auto piece = static_cast<int>(board.at(square).type().internal());
    return table[piece][mobility(board, square, color)];
}

int
evaluateMiddleGame(Board & board, const Color color) {
    auto piece_value_mg = [](Board & board, const Color color) {
        auto indices = board.us(color) & // Only index our side, not enemy side
                       board.pieces(PieceType::PAWN,
                                    PieceType::KNIGHT,
                                    PieceType::BISHOP,
                                    PieceType::ROOK,
                                    PieceType::QUEEN);
        auto score = 0;

        while (!indices.empty()) {
            const auto index = indices.msb();
            score += piece_value_bonus(board, index, true);
            indices.clear(index);
        }

        return score;
    };

    auto piece_square_table_mg = [](Board & board, const Color color) {
        auto indices = board.us(color) & board.pieces(PieceType::PAWN,
                                                      PieceType::KNIGHT,
                                                      PieceType::BISHOP,
                                                      PieceType::ROOK,
                                                      PieceType::QUEEN,
                                                      PieceType::KING);
        auto score = 0;

        while (!indices.empty()) {
            const auto index = indices.msb();
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

    auto mobility_mg = [](Board & board, const Color color) {
        auto indices =
            board.us(color) &
            board.pieces(PieceType::KNIGHT, PieceType::BISHOP, PieceType::ROOK, PieceType::QUEEN);
        auto score = 0;

        while (!indices.empty()) {
            const auto index = indices.msb();

            score += mobility_bonus(board, index, color, true);

            indices.clear(index);
        }

        return score;
    };

    auto score = 0;
    score += piece_value_mg(board, color);
    score += piece_square_table_mg(board, color);
    score += imbalance_total(board, color);
    score += pawns_mg(board, color);
    // score += pieces_mg(board, color);
    score += mobility_mg(board, color);

    return score;
}

int
evaluateEndGame(Board & board, const Color color) {
    return 0;
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
evaluateStockfish(Board & board) {
    auto eval = [&board](Color color) -> int {
        auto mg = evaluateMiddleGame(board, color);
        auto eg = evaluateEndGame(board, color);
        std::cerr << color << " mg: " << mg << "\n" << std::flush;

        auto ph = phase(board);
        auto rule50 = board.halfMoveClock();

        // eg = eg * scale_factor(board, eg) / 64;
        auto score = (((mg * ph + ((eg * (128 - ph)) << 0)) / 128) << 0);

        score += board.sideToMove() == color ? 28 : 0; // tempo
        score = (score * (100 - rule50) / 100) << 0;
        return score;
    };

    const auto turn = board.sideToMove() == Color::WHITE;
    const auto score = eval(Color::WHITE) - eval(Color::BLACK);
    return turn ? score : -score;
}

} // namespace segfault
