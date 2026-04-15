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

Segfault::Segfault() {
    loadWeights(weights_, "weights.bin");
}

int
Segfault::evaluateNetwork(const Board & board) {
    const auto & acc = accumulator_stack_.back();
    const auto   pred = forward_from_accumulator(weights_, acc);
    const auto   eval = static_cast<int>((pred - 0.5f) * 10000.0f);

    return board.sideToMove() == Color::WHITE ? eval : -eval;
}

Move
Segfault::search(Board & board, std::size_t wtime, std::size_t btime, std::size_t winc,
                 std::size_t binc, std::atomic<bool> & stop) {
    const auto time_estimate = [](Board & board, Movelist & moves, std::size_t wtime,
                                  std::size_t btime) -> auto {
        const auto side_time = board.sideToMove() == Color::WHITE ? wtime : btime;
        const auto moves_left = std::max(60 - static_cast<int>(board.fullMoveNumber()), 10);

        constexpr std::size_t increment = 1000; // example 1s increment
        constexpr std::size_t increment_safety_margin = 300; // keep some back
        const auto            usable_increment =
            increment > increment_safety_margin ? increment - increment_safety_margin : 0;

        const double branching_factor_weight =
            std::clamp(static_cast<double>(moves.size()) / 20.0, 0.5, 2.0);
        const auto max_alloc =
            static_cast<std::size_t>(side_time * 0.2); // Never spend >20% of time

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
    const auto deadline = start + std::chrono::milliseconds(time_allocated);
    accumulator_stack_.clear();
    accumulator_stack_.emplace_back();
    accumulator_stack_.back().refresh(weights_, encode_board(board).data());

    for (auto & row : history_)
        for (auto & val : row)
            val /= 2;

    auto best_move = moves[0];
    auto previous_score = 0;

    for (auto d = 1; d <= 32; d++) {
        auto alpha = -INT32_MAX;
        auto beta = INT32_MAX;
        auto delta = 500;

        if (d >= 4) {
            alpha = previous_score - delta;
            beta = previous_score + delta;
        }

        auto iteration_aborted = false;

        while (true) {
            auto current_alpha = alpha;
            auto iteration_best_score = -INT32_MAX;
            auto iteration_best_move = moves[0];

            for (auto i = 0; i < moves.size(); ++i) {
                const auto move = moves[i];
                makeMoveAcc(board, move);

                int score;
                if (i == 0) {
                    score = -pvs(board, -beta, -current_alpha, d - 1, 1);
                } else {
                    score = -pvs(board, -current_alpha - 1, -current_alpha, d - 1, 1);
                    if (score > current_alpha && score < beta)
                        score = -pvs(board, -beta, -current_alpha, d - 1, 1);
                }

                unmakeMoveAcc(board, move);

                if (stop || std::chrono::system_clock::now() > deadline) {
                    iteration_aborted = true;
                    break;
                }

                if (score > iteration_best_score) {
                    iteration_best_score = score;
                    iteration_best_move = move;

                    if (score > current_alpha)
                        current_alpha = score;

                    pv_table_.moves[0][0] = move;
                    for (auto j = 1; j < pv_table_.length[1]; j++)
                        pv_table_.moves[0][j] = pv_table_.moves[1][j];
                    pv_table_.length[0] = pv_table_.length[1];
                }
            }

            if (iteration_aborted)
                break;

            if (iteration_best_score <= alpha) {
                alpha -= delta;
                delta *= 2;
                continue;
            }
            if (iteration_best_score >= beta) {
                beta += delta;
                delta *= 2;
                continue;
            }

            best_move = iteration_best_move;
            previous_score = iteration_best_score;
            break;
        }

        if (iteration_aborted)
            break;

        const auto print_pv = [this]() {
            for (auto i = 0; i < pv_table_.length[0]; i++) {
                if (i > 0)
                    std::cout << ' ';
                std::cout << uci::moveToUci(pv_table_.moves[0][i]);
            }
        };

        // std::cout << "info depth " << d << " score cp " << previous_score << " pv ";
        // print_pv();
        // std::cout << std::endl;

        if (stop)
            break;
    }

    // auto end = std::chrono::system_clock::now();
    // auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    // std::cout << elapsed.count() << "ms" << std::endl;

    return best_move;
}

Move
Segfault::search(Board & board, uint8_t depth, std::atomic<bool> & stop) {
    Movelist moves;
    generateAllMoves(board, moves);

    const auto start = std::chrono::system_clock::now();
    accumulator_stack_.clear();
    accumulator_stack_.emplace_back();
    accumulator_stack_.back().refresh(weights_, encode_board(board).data());

    for (auto & row : history_)
        for (auto & val : row)
            val /= 2;

    auto best_move = moves[0];
    auto previous_score = 0;

    for (auto d = 1; d <= depth; d++) {
        auto alpha = -INT32_MAX;
        auto beta = INT32_MAX;
        auto delta = 500;

        if (d >= 4) {
            alpha = previous_score - delta;
            beta = previous_score + delta;
        }

        auto iteration_aborted = false;

        while (true) {
            auto current_alpha = alpha;
            auto iteration_best_score = -INT32_MAX;
            auto iteration_best_move = moves[0];

            for (auto i = 0; i < moves.size(); ++i) {
                const auto move = moves[i];
                makeMoveAcc(board, move);

                int score;
                if (i == 0) {
                    score = -pvs(board, -beta, -current_alpha, d - 1, 1);
                } else {
                    score = -pvs(board, -current_alpha - 1, -current_alpha, d - 1, 1);
                    if (score > current_alpha && score < beta)
                        score = -pvs(board, -beta, -current_alpha, d - 1, 1);
                }

                unmakeMoveAcc(board, move);
                // std::cout << "move: " << uci::moveToUci(move) << " score: " << score << std::endl;

                if (stop) {
                    iteration_aborted = true;
                    break;
                }

                if (score > iteration_best_score) {
                    iteration_best_score = score;
                    iteration_best_move = move;

                    if (score > current_alpha)
                        current_alpha = score;

                    pv_table_.moves[0][0] = move;
                    for (auto j = 1; j < pv_table_.length[1]; j++)
                        pv_table_.moves[0][j] = pv_table_.moves[1][j];
                    pv_table_.length[0] = pv_table_.length[1];
                }
            }

            if (iteration_aborted)
                break;

            if (iteration_best_score <= alpha) {
                alpha -= delta;
                delta *= 2;
                continue;
            }
            if (iteration_best_score >= beta) {
                beta += delta;
                delta *= 2;
                continue;
            }

            best_move = iteration_best_move;
            previous_score = iteration_best_score;
            break;
        }

        if (iteration_aborted)
            break;

        const auto print_pv = [this]() {
            for (auto i = 0; i < pv_table_.length[0]; i++) {
                if (i > 0)
                    std::cout << ' ';
                std::cout << uci::moveToUci(pv_table_.moves[0][i]);
            }
        };

        std::cout << "info depth " << d << " score cp " << previous_score << " pv ";
        print_pv();
        std::cout << std::endl;

        if (stop)
            break;
    }

    auto end = std::chrono::system_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    std::cout << elapsed.count() << "ms" << std::endl;

    return best_move;
}

} // namespace segfault
