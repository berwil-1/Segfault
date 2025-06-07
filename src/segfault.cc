#include "segfault.hh"

#include "search.hh"
#include "util.hh"

#include <chrono>
#include <string>
#include <unordered_map>

namespace {
using chess::Board;
using chess::Color;

auto
time_allocated_func(const Board & board, const std::size_t wtime, const std::size_t btime) {
    // constexpr auto time_allocated_lowest = std::ptrdiff_t{1000}; // ms

    // std::cout << "wtime: " << wtime << "\n";
    const auto side_time = board.sideToMove() == Color::WHITE ? wtime : btime;
    // std::cout << "side_time: " << side_time << "\n";
    const auto moves_left = std::max(80 - static_cast<int>(board.fullMoveNumber()), 10);
    // std::cout << "moves_left: " << moves_left << "\n";
    const auto time_allocated_lowest = static_cast<std::ptrdiff_t>(side_time / moves_left); // ms
    // std::cout << "time_allocated_lowest: " << time_allocated_lowest << "\n";

    // How much more time do we have compared to the other side?
    const auto time_difference =
        static_cast<std::ptrdiff_t>(wtime) - static_cast<std::ptrdiff_t>(btime);
    // std::cout << "time_difference: " << time_difference << "\n";
    const auto side_time_difference = board.sideToMove() == Color::WHITE ? time_difference
                                                                         : -time_difference;
    // std::cout << "side_time_difference: " << side_time_difference << "\n";

    // Make sure we don't spend negative time, we should at least be able to spend
    // the amount gained per move (if any given), otherwise roughly a
    return static_cast<std::size_t>(std::max(side_time_difference, time_allocated_lowest));
}
} // namespace

namespace segfault {

static_assert(sizeof(TranspositionTableEntry) == 16);
static_assert(alignof(TranspositionTableEntry) == 4);
static_assert(std::is_trivially_copyable_v<TranspositionTableEntry>);

Move
Segfault::search(Board & board, std::size_t wtime, std::size_t btime, uint16_t depth) {
    std::unordered_map<uint16_t, int> evals;
    // auto                          highscore = -INT32_MAX;
    Movelist moves;
    // Move                          bestmove;
    generateAllMoves(moves, board);

    const auto time_allocated = time_allocated_func(board, wtime, btime);
    // std::cout << "time: " << time << "\n";
    auto start = std::chrono::system_clock::now();

    for (auto d = depth;; d++) {
        for (Move move : moves) {
            if (std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::system_clock::now() - start)
                    .count() > 2000) {
                break;
            }
            board.makeMove(move);
            const auto score = -negaAlphaBeta(board, -INT32_MAX, INT32_MAX, d);
            evals.emplace(move.move(), score);
            // std::cout << move << ": " << score << std::endl;

            board.unmakeMove(move);

            /*if (highscore <= score) {
                highscore = score;
                bestmove = move;
            }*/
        }
        if (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() -
                                                                  start)
                .count() > 2000) {
            break;
        }
    }

    // std::cout << "size: " << transposition_table_.size() << std::endl;

    auto best = std::max_element(evals.begin(), evals.end(), [](const auto & a, const auto & b) {
        return a.second < b.second;
    });

    return (*best).first;
}

} // namespace segfault
