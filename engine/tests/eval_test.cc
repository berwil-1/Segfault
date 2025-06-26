#include "eval.hh"

#include "chess.hh"

#include <gtest/gtest.h>
#include <iostream>

using namespace chess;
using namespace segfault;

struct ExpectedParam {
    std::string fen;
    int         expected;
};

struct ExpectedPerSquareParam {
    std::string      fen;
    std::vector<int> expected;
};

struct ColorExpectedParam {
    std::string fen;
    Color       color;
    int         expected;
};

struct ColorExpectedPerSquareParam {
    std::string      fen;
    Color            color;
    std::vector<int> expected;
};

class EvalTest : public ::testing::TestWithParam<ExpectedParam> {};

TEST_P(EvalTest, MiddleGame) {
    const auto & [fen, expected] = GetParam();
    Board board = Board::fromFen(fen);
    EXPECT_EQ(evaluateMiddleGame(board), expected);
}

INSTANTIATE_TEST_SUITE_P(EvalCases, EvalTest,
                         ::testing::Values(ExpectedParam{
                             "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", 0}));

class StrengthSquareTest : public ::testing::TestWithParam<ColorExpectedPerSquareParam> {};

TEST_P(StrengthSquareTest, MiddleGame) {
    const auto & [fen, color, expected] = GetParam();
    Board board = Board::fromFen(fen);

    for (int index = 0; index < 64; index++) {
        EXPECT_EQ(strength_square(board, color, index), expected.at(index));
    }

    /*for (int y = 0; y < 8; y++) {
        for (int x = 0; x < 8; x++) {
            std::cerr << strength_square(board, color, y * 8 + x) << " ";
        }
        std::cerr << std::endl;
    }

    throw std::runtime_error{""};*/
}

INSTANTIATE_TEST_SUITE_P(
    DISABLED_EvalCases, StrengthSquareTest,
    ::testing::Values(
        ColorExpectedPerSquareParam{
            "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", Color::WHITE,
            std::vector<int>{
                -54, -54, -87, -83, -83, -87, -54, -54, -54, -54, -87, -83, -83, -87, -54, -54,
                -54, -54, -87, -83, -83, -87, -54, -54, -54, -54, -87, -83, -83, -87, -54, -54,
                -54, -54, -87, -83, -83, -87, -54, -54, -54, -54, -87, -83, -83, -87, -54, -54,
                222, 222, 128, 54,  54,  128, 222, 222, 222, 222, 128, 54,  54,  128, 222, 222,
            }},
        ColorExpectedPerSquareParam{
            "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", Color::BLACK,
            std::vector<int>{
                222, 222, 128, 54,  54,  128, 222, 222, 222, 222, 128, 54,  54,  128, 222, 222,
                -54, -54, -87, -83, -83, -87, -54, -54, -54, -54, -87, -83, -83, -87, -54, -54,
                -54, -54, -87, -83, -83, -87, -54, -54, -54, -54, -87, -83, -83, -87, -54, -54,
                -54, -54, -87, -83, -83, -87, -54, -54, -54, -54, -87, -83, -83, -87, -54, -54,
            }}));

class ShelterStrengthTest : public ::testing::TestWithParam<ColorExpectedParam> {};

class ShelterStormTest : public ::testing::TestWithParam<ColorExpectedParam> {};

TEST_P(ShelterStrengthTest, ShelterStrength) {
    const auto & [fen, color, expected] = GetParam();
    Board  board = Board::fromFen(fen);
    Square king_sq = board.kingSq(color);
    EXPECT_EQ(shelter_strength(board, king_sq, color), expected);
}

TEST_P(ShelterStormTest, ShelterStorm) {
    const auto & [fen, color, expected] = GetParam();
    Board  board = Board::fromFen(fen);
    Square king_sq = board.kingSq(color);
    EXPECT_EQ(shelter_storm(board, king_sq, color), expected);
}

INSTANTIATE_TEST_SUITE_P(
    DISABLED_ShelterCases, ShelterStrengthTest,
    ::testing::Values(ColorExpectedParam{"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
                                         Color::WHITE, 222},
                      ColorExpectedParam{"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
                                         Color::BLACK, 222}));

INSTANTIATE_TEST_SUITE_P(
    DISABLED_ShelterCases, ShelterStormTest,
    ::testing::Values(ColorExpectedParam{"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
                                         Color::WHITE, 56},
                      ColorExpectedParam{"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
                                         Color::BLACK, 56}));

// Entry point for Google Test
int
main(int argc, char ** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
