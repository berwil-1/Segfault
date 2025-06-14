#include "segfault.hh"

#include "eval.hh"
#include "search.hh"
#include "util.hh"

#include <chrono>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace {
using chess::Board;
using chess::Color;
using chess::Movelist;

auto
time_allocated_func(const Board &     board,
                    const Movelist &  moves,
                    const std::size_t wtime,
                    const std::size_t btime) {
    const auto side_time = board.sideToMove() == Color::WHITE ? wtime : btime;
    const auto moves_left = std::max(60 - static_cast<int>(board.fullMoveNumber()), 10);
    // const auto time_allocated_lowest = static_cast<std::ptrdiff_t>(side_time / moves_left); // ms

    constexpr std::size_t increment = 1000; // example 1s increment
    constexpr std::size_t increment_safety_margin = 300; // keep some back
    const auto            usable_increment =
        increment > increment_safety_margin ? increment - increment_safety_margin : 0;

    const double branching_factor_weight =
        std::clamp(static_cast<double>(moves.size()) / 30.0, 0.5, 1.5);
    const auto max_alloc = static_cast<std::size_t>(side_time * 0.3); // Never spend >30% of time

    auto time_allocated_raw = side_time / moves_left;
    time_allocated_raw += usable_increment;
    time_allocated_raw = static_cast<std::size_t>(time_allocated_raw * branching_factor_weight);
    time_allocated_raw = std::max(time_allocated_raw, static_cast<std::size_t>(50)); // never give 0

    return std::min(time_allocated_raw, max_alloc);

    /*// How much more time do we have compared to the other side?
    const auto time_difference =
        static_cast<std::ptrdiff_t>(wtime) - static_cast<std::ptrdiff_t>(btime);
    const auto side_time_difference = board.sideToMove() == Color::WHITE ? time_difference
                                                                         : -time_difference;

    const auto time_allocated = side_time_difference;

    // Make sure we don't spend negative time, we should at least be able to spend
    // the amount gained per move (if any given), otherwise roughly a
    return static_cast<std::size_t>(std::max(time_allocated, time_allocated_lowest));*/
}
} // namespace

namespace segfault {

static_assert(sizeof(TranspositionTableEntry) == 16);
static_assert(alignof(TranspositionTableEntry) == 4);
static_assert(std::is_trivially_copyable_v<TranspositionTableEntry>);

Move
Segfault::search(Board & board, std::size_t wtime, std::size_t btime, uint16_t depth) {
    std::vector<std::pair<uint16_t, int>> evals;
    Movelist                              moves;
    generateAllMoves(moves, board);

    for (const auto move : moves) {
        evals.emplace_back(move.move(), 0);
    }

    auto time_allocated = time_allocated_func(board, moves, wtime, btime);

    // std::cout << "time: " << time_allocated << "\n";
    auto           start = std::chrono::system_clock::now();
    constexpr auto depth_max = 32;

    // std::cout << "fen: " << board.getFen() << "\n";

    for (auto d = depth; d < depth_max; d++) {
        std::cout << "info "
                  << "depth " << d << " score cp " << evaluateStockfish(board) << " time "
                  << std::chrono::duration_cast<std::chrono::milliseconds>(
                         std::chrono::system_clock::now() - start)
                         .count()
                  << std::endl;

        for (auto & eval : evals) {
            if (std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::system_clock::now() - start)
                    .count() > time_allocated) {
                break;
            }
            board.makeMove(eval.first);
            const auto score = -negaAlphaBeta(board, -INT32_MAX, INT32_MAX, d);
            eval.second = score;
            // std::cout << Move{eval.first} << ": " << score << std::endl;

            board.unmakeMove(eval.first);
        }

        std::sort(evals.begin(), evals.end(), [](const auto & a, const auto & b) {
            return a.second > b.second;
        });

        if (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() -
                                                                  start)
                .count() > time_allocated) {
            break;
        }
    }

    return evals.front().first;
}

} // namespace segfault
