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
    const auto deadline =
        std::chrono::system_clock::now() + std::chrono::milliseconds(time_allocated);

    auto best_move = moves[0];

    for (auto d = 1; d <= 32; d++) {
        auto       iteration_best_score = -INT32_MAX;
        auto       iteration_best_move = moves[0];
        auto       alpha = -INT32_MAX;
        const auto beta = INT32_MAX;
        auto       aborted = false;

        for (auto i = 0; i < moves.size(); ++i) {
            const auto move = moves[i];
            board.makeMove(move);

            int score;
            if (i == 0) {
                score = -pvs(board, -beta, -alpha, d - 1, 1);
            } else {
                score = -pvs(board, -alpha - 1, -alpha, d - 1, 1);
                if (score > alpha && score < beta)
                    score = -pvs(board, -beta, -alpha, d - 1, 1);
            }

            board.unmakeMove(move);

            if (stop || std::chrono::system_clock::now() > deadline) {
                aborted = true;
                break;
            }

            if (score > iteration_best_score) {
                iteration_best_score = score;
                iteration_best_move = move;

                if (score > alpha)
                    alpha = score;

                pv_table_.moves[0][0] = move;
                for (auto j = 1; j < pv_table_.length[1]; j++)
                    pv_table_.moves[0][j] = pv_table_.moves[1][j];
                pv_table_.length[0] = pv_table_.length[1];
            }
        }

        if (aborted)
            break;

        best_move = iteration_best_move;

        const auto print_pv = [this]() {
            for (auto i = 0; i < pv_table_.length[0]; i++) {
                if (i > 0)
                    std::cout << ' ';
                std::cout << uci::moveToUci(pv_table_.moves[0][i]);
            }
        };
        std::cout << "info depth " << d << " score cp " << iteration_best_score << " pv ";
        print_pv();
        std::cout << std::endl;
    }

    return best_move;
}

} // namespace segfault
