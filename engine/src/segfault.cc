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
    loadWeights(network_weights_, "weights-20260501.bin");
}

int
Segfault::evaluateNetwork(const Board & board, const SearchContext & ctx) {
    const auto & acc = ctx.accumulator_stack.back();
    const auto   pred = forward_from_accumulator(network_weights_, acc);
    // const auto   eval = static_cast<int>((pred - 0.5f) * 10000.0f);
    const auto eval = std::clamp(static_cast<int>((pred - 0.5f) * 10000.0f), -100000, 100000);

    return board.sideToMove() == Color::WHITE ? eval : -eval;
}

Move
Segfault::search(Board & board, std::size_t wtime, std::size_t btime, std::size_t winc,
                 std::size_t binc, std::atomic<bool> & stop) {
    const auto time_estimate = [](Board & board, Movelist & moves, std::size_t wtime,
                                  std::size_t btime, std::size_t winc, std::size_t binc) -> auto {
        const auto side_time = board.sideToMove() == Color::WHITE ? wtime : btime;
        const auto side_inc = board.sideToMove() == Color::WHITE ? winc : binc;
        const auto moves_left = std::max(80 - static_cast<int>(board.fullMoveNumber()), 5);

        constexpr std::size_t increment_safety_margin = 300;
        const auto            usable_increment =
            side_inc > increment_safety_margin ? side_inc - increment_safety_margin : 0;
        const auto branching_factor_weight = std::clamp(moves.size() / 10.0f, 1.0f, 3.0f);
        auto       max_alloc = static_cast<std::size_t>(side_time / 5); // Never spend >20% of time

        // Emergency time handling
        if (side_time < 1000) {
            max_alloc = std::size_t{50};
        } else if (side_time < 5000) {
            max_alloc = std::min(max_alloc, side_time / 15);
        } else if (side_time < 15000) {
            max_alloc = std::min(max_alloc, side_time / 10);
        }

        auto time_allocated_raw =
            static_cast<std::size_t>((side_time / moves_left) * branching_factor_weight);
        time_allocated_raw += usable_increment;
        time_allocated_raw =
            std::clamp(time_allocated_raw, static_cast<std::size_t>(100), max_alloc);
        return time_allocated_raw;
    };

    Movelist moves;
    generateAllMoves(board, moves);

    const auto time_allocated = time_estimate(board, moves, wtime, btime, winc, binc);
    const auto start = std::chrono::system_clock::now();
    deadline_ = start + std::chrono::milliseconds(time_allocated);
    search_aborted_ = false;

    std::vector<std::pair<int, Move>> scored_moves;
    accumulator_stack_.clear();
    accumulator_stack_.emplace_back();
    accumulator_stack_.back().refresh(network_weights_, encode_board(board).data());

    // History heuristics, decay
    for (auto & row : history_) {
        for (auto & val : row)
            val /= 2;
    }

    for (const auto move : moves)
        scored_moves.emplace_back(0, move);

    auto best_move = moves[0];
    auto best_move_changes = 0;
    auto previous_score = 0;
    auto stable_count = 0;

    auto score_drop = false;
    auto found_mate = false;

    for (auto d = 1; d <= 32; d++) {
        auto alpha = -INT32_MAX;
        auto beta = INT32_MAX;
        auto delta = int{500};
        search_aborted_ = false;

        if (d >= 4) {
            alpha = previous_score - delta;
            beta = previous_score + delta;
        }

        const auto prev_best = best_move;
        auto       iteration_aborted = std::atomic<bool>{false};
        auto       mx = std::mutex{};

        while (true) {
            auto current_alpha = std::atomic<int>{alpha};
            auto iteration_best_score = -INT32_MAX;
            auto iteration_best_move = scored_moves[0].second;
            auto iteration_best_pv = PVTable{};
            auto futures = std::vector<std::future<void>>{};

            auto contexts = std::vector<SearchContext>{scored_moves.size()};
            for (auto & ctx : contexts) {
                ctx.history = history_;
                ctx.accumulator_stack = accumulator_stack_;
            }

            for (auto i = std::size_t{0}; i < scored_moves.size(); i++) {
                futures.push_back(thread_pool_.enqueue(
                    [this, &scored_moves, &contexts, &current_alpha, &stop, &iteration_aborted,
                     &iteration_best_score, &iteration_best_move, &iteration_best_pv, &mx, board,
                     beta, d, i]() mutable {
#ifndef MULTI_THREADING
                        const std::scoped_lock lock{mx};
#endif

                        auto &     ctx = contexts[i];
                        const auto move = scored_moves[i].second;
                        makeMoveAcc(board, ctx, move); // mutates ctx, not this

                        int score;
                        if (i == 0) {
                            score = -pvs(board, ctx, -beta, -current_alpha, d - 1, 1);
                        } else {
                            score = -pvs(board, ctx, -current_alpha - 1, -current_alpha, d - 1, 1);
                            if (score > current_alpha && score < beta) {
                                score = -pvs(board, ctx, -beta, -current_alpha, d - 1, 1);
                            }
                        }

                        unmakeMoveAcc(board, ctx, move);
                        // std::cout << "move: " << uci::moveToUci(move) << " score: " << score
                        //           << std::endl;

                        if (stop || search_aborted_) {
                            iteration_aborted = true;
                            return;
                        }

#ifdef MULTI_THREADING
                        {
                            const std::scoped_lock lock{mx};
#endif
                            if (score > iteration_best_score) {
                                iteration_best_score = score;
                                iteration_best_move = move;

                                auto prev = current_alpha.load();
                                while (score > prev &&
                                       !current_alpha.compare_exchange_weak(prev, score)) {
                                }

                                iteration_best_pv.moves[0][0] = move;
                                for (auto j = 1; j < ctx.pv_table.length[1]; j++)
                                    iteration_best_pv.moves[0][j] = ctx.pv_table.moves[1][j];
                                iteration_best_pv.length[0] = ctx.pv_table.length[1];
                            }
#ifdef MULTI_THREADING
                        }
#endif
                        scored_moves[i].first = score;
                    }));
            }

            for (auto & future : futures)
                future.wait();

            // Aggregate node count
            for (const auto & ctx : contexts)
                nodes_ += ctx.nodes;

            // Commit winning PV
            pv_table_ = iteration_best_pv;

            if (iteration_aborted)
                break;

            if (d > 1 && iteration_best_move != best_move)
                best_move_changes++;
            if (d > 1 && iteration_best_score < previous_score - 500)
                score_drop = true;
            if (iteration_best_score > SCORE_MATE - 200) {
                found_mate = true;
                best_move = iteration_best_move;
                previous_score = iteration_best_score;
                break;
            }

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

        if (best_move == prev_best) {
            stable_count++;
        } else {
            stable_count = 0;
        }

        if (d > 1 && best_move != prev_best)
            best_move_changes++;

        std::sort(scored_moves.begin(), scored_moves.end(),
                  [](const auto & a, const auto & b) { return a.first > b.first; });

        const auto print_pv = [this]() {
            for (auto i = 0; i < pv_table_.length[0]; i++) {
                if (i > 0) {
                    std::cout << ' ';
                    logs_ << ' ';
                }
                std::cout << uci::moveToUci(pv_table_.moves[0][i]);
                logs_ << uci::moveToUci(pv_table_.moves[0][i]);
            }
        };

        std::cout << "info depth " << d << " score cp " << previous_score << " pv ";
        logs_ << "Segfault::search(): info depth " << d << " score cp " << previous_score << " pv ";
        print_pv();
        std::cout << std::endl;
        logs_ << std::endl;

        auto time_multiplier = 1.0f + best_move_changes / 8.0f + (score_drop ? 0.5f : 0.0f);
        deadline_ =
            start + std::chrono::milliseconds(static_cast<int>(time_allocated * time_multiplier));

        const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                                 std::chrono::system_clock::now() - start)
                                 .count();
        const auto effective_time = static_cast<std::size_t>(time_allocated * time_multiplier);

        if (iteration_aborted) {
            logs_ << "Segfault::search(): iteration_aborted=true d=" << d
                  << " search_aborted=" << search_aborted_ << " stop=" << stop.load() << " elapsed="
                  << std::chrono::duration_cast<std::chrono::milliseconds>(
                         std::chrono::system_clock::now() - start)
                         .count()
                  << " deadline_ms=" << time_allocated << std::endl;
        }
        if (found_mate) {
            logs_ << "Segfault::search(): found_mate=true" << std::endl;
            break;
        }
        if (stable_count >= 2 && elapsed > effective_time / 3) {
            logs_ << "Segfault::search(): stable_count >= 2 && elapsed > effective_time / 3 "
                     "stable_count="
                  << stable_count << " elasped=" << elapsed << " effective_time=" << effective_time
                  << std::endl;
            break;
        }
        if (elapsed > effective_time / 2) {
            logs_ << "Segfault::search(): elapsed > effective time / 2 elasped=" << elapsed
                  << " effective_time=" << effective_time << std::endl;
            break;
        }
        if (stop) {
            logs_ << "Segfault::search(): stop=true" << std::endl;
            break;
        }
    }

    auto end = std::chrono::system_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    std::cout << elapsed.count() << "ms" << std::endl;
    logs_ << "Segfault::search(): " << elapsed.count() << "ms" << std::endl;

    return best_move;
}

Move
Segfault::search(Board & board, uint8_t depth, std::atomic<bool> & stop) {
    Movelist moves;
    generateAllMoves(board, moves);

    const auto start = std::chrono::system_clock::now();
    search_aborted_ = false;

    std::vector<std::pair<int, Move>> scored_moves;
    accumulator_stack_.clear();
    accumulator_stack_.emplace_back();
    accumulator_stack_.back().refresh(network_weights_, encode_board(board).data());

    // History heuristics, decay
    for (auto & row : history_) {
        for (auto & val : row)
            val /= 2;
    }

    for (const auto move : moves)
        scored_moves.emplace_back(0, move);

    auto best_move = moves[0];
    auto previous_score = 0;

    for (auto d = 1; d <= depth; d++) {
        auto alpha = -INT32_MAX;
        auto beta = INT32_MAX;
        auto delta = int{500};

        if (d >= 4) {
            alpha = previous_score - delta;
            beta = previous_score + delta;
        }

        auto iteration_aborted = std::atomic<bool>{false};
        auto mx = std::mutex{};

        while (true) {
            auto current_alpha = std::atomic<int>{alpha};
            auto iteration_best_score = -INT32_MAX;
            auto iteration_best_move = scored_moves[0].second;
            auto iteration_best_pv = PVTable{};
            auto futures = std::vector<std::future<void>>{};

            auto contexts = std::vector<SearchContext>{scored_moves.size()};
            for (auto & ctx : contexts) {
                ctx.history = history_;
                ctx.accumulator_stack = accumulator_stack_;
            }

            for (auto i = std::size_t{0}; i < scored_moves.size(); i++) {
                futures.push_back(thread_pool_.enqueue(
                    [this, &scored_moves, &contexts, &current_alpha, &stop, &iteration_aborted,
                     &iteration_best_score, &iteration_best_move, &iteration_best_pv, &mx, board,
                     beta, d, i]() mutable {
#ifndef MULTI_THREADING
                        const std::scoped_lock lock{mx};
#endif

                        auto &     ctx = contexts[i];
                        const auto move = scored_moves[i].second;
                        makeMoveAcc(board, ctx, move); // mutates ctx, not this

                        int score;
                        if (i == 0) {
                            score = -pvs(board, ctx, -beta, -current_alpha, d - 1, 1);
                        } else {
                            score = -pvs(board, ctx, -current_alpha - 1, -current_alpha, d - 1, 1);
                            if (score > current_alpha && score < beta) {
                                score = -pvs(board, ctx, -beta, -current_alpha, d - 1, 1);
                            }
                        }

                        unmakeMoveAcc(board, ctx, move);
                        // std::cout << "move: " << uci::moveToUci(move) << " score: " << score
                        //           << std::endl;

                        if (stop || search_aborted_) {
                            iteration_aborted = true;
                            return;
                        }

#ifdef MULTI_THREADING
                        {
                            const std::scoped_lock lock{mx};
#endif
                            if (score > iteration_best_score) {
                                iteration_best_score = score;
                                iteration_best_move = move;

                                auto prev = current_alpha.load();
                                while (score > prev &&
                                       !current_alpha.compare_exchange_weak(prev, score)) {
                                }

                                iteration_best_pv.moves[0][0] = move;
                                for (auto j = 1; j < ctx.pv_table.length[1]; j++)
                                    iteration_best_pv.moves[0][j] = ctx.pv_table.moves[1][j];
                                iteration_best_pv.length[0] = ctx.pv_table.length[1];
                            }
#ifdef MULTI_THREADING
                        }
#endif
                        scored_moves[i].first = score;
                    }));
            }

            for (auto & future : futures)
                future.wait();

            // Aggregate node count
            for (const auto & ctx : contexts)
                nodes_ += ctx.nodes;

            // Commit winning PV
            pv_table_ = iteration_best_pv;

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

        std::sort(scored_moves.begin(), scored_moves.end(),
                  [](const auto & a, const auto & b) { return a.first > b.first; });

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
