#include "segfault.hh"

#include "eval.hh"
#include "search.hh"
#include "util.hh"

#include <array>
#include <chrono>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace segfault {

Move
Segfault::search(Board & board, std::size_t wtime, std::size_t btime, std::atomic<bool> & stop) {
    const auto time_estimate = [](Board & board, Movelist & moves, std::size_t wtime,
                                  std::size_t btime) -> auto {
        const auto side_time = board.sideToMove() == Color::WHITE ? wtime : btime;
        const auto moves_left = std::max(60 - static_cast<int>(board.fullMoveNumber()), 10);
        // const auto time_allocated_lowest = static_cast<std::ptrdiff_t>(side_time / moves_left);
        // // ms

        constexpr std::size_t increment = 1000; // example 1s increment
        constexpr std::size_t increment_safety_margin = 300; // keep some back
        const auto            usable_increment =
            increment > increment_safety_margin ? increment - increment_safety_margin : 0;

        const double branching_factor_weight =
            std::clamp(static_cast<double>(moves.size()) / 30.0, 0.5, 1.5);
        const auto max_alloc =
            static_cast<std::size_t>(side_time * 0.3); // Never spend >30% of time

        auto time_allocated_raw = side_time / moves_left;
        time_allocated_raw += usable_increment;
        time_allocated_raw = static_cast<std::size_t>(time_allocated_raw * branching_factor_weight);
        time_allocated_raw =
            std::max(time_allocated_raw, static_cast<std::size_t>(50)); // never give 0

        return std::min(time_allocated_raw, max_alloc);
    };

    Movelist moves;
    generateAllMoves(board, moves);
    const auto time_allocated = time_estimate(board, moves, wtime, btime);
    const auto start = std::chrono::system_clock::now();
    auto       queue = std::priority_queue<std::pair<int, int>>{};
    auto       latest_queue = std::priority_queue<std::pair<int, int>>{};

    for (auto d = 1; d <= 32; d++) {
        queue = latest_queue;
        latest_queue = {};

        // estimated time taken in ms at depth d
        // const auto time_estimate_depth = static_cast<int>(4.071384f * std::powf(d, 5.765545f));
        const auto time_estimate_depth = static_cast<int>(1.246391f * std::powf(d, 5.995528f));
        const auto time_since_start = std::chrono::duration_cast<std::chrono::milliseconds>(
                                          std::chrono::system_clock::now() - start)
                                          .count();

        if (time_since_start + time_estimate_depth > time_allocated * 2.0f) {
            break;
        }
        std::cout << "d: " << d << std::endl;

        for (auto & move : moves) {
            board.makeMove(move);
            const auto score = -pvs(board, -INT32_MAX, INT32_MAX, d);
            board.unmakeMove(move);
            latest_queue.emplace(score, move.move());
            std::cout << uci::moveToUci(move) << ": " << score << std::endl;
            if (stop)
                break;
        }

        // TODO:
        // const auto top = latest_queue.top();
        // latest_queue.pop();
        // while(latest_queue.top() == top)
        // latest_queue.pop();

        if (stop)
            break;
    }

    auto end = std::chrono::system_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    std::cout << elapsed.count() << "ms" << std::endl;
    std::cout << "Move: " << uci::moveToUci(queue.top().second) << std::endl;

    return queue.top().second;
}

} // namespace segfault
