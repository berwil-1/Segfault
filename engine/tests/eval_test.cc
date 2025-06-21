#include "eval.hh"

#include "chess.hh"

#include <gtest/gtest.h>

using namespace chess;
using namespace segfault;

TEST(EvalTest, Zero) {
    Board board = Board::fromFen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    EXPECT_EQ(evaluateMiddleGame(board), 0);
}

// Entry point for Google Test
int
main(int argc, char ** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
