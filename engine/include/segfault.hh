#pragma once

#include "chess.hh"
#include "eval.hh"
#include "matmul.hh"
// #include "torch/torch.h"

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <future>
#include <mutex>
#include <queue>
#include <thread>
#include <unordered_map>
#include <utility>
#include <vector>

namespace segfault {

static constexpr auto MAX_PLY{128};

using namespace chess;
using Accumulators = std::vector<Accumulator>;
using Killers = std::array<std::array<Move, 2>, MAX_PLY>;
using History = std::array<std::array<int, 64>, 12>;

struct TranspositionTableEntry {
    enum Bound : uint16_t { EXACT, LOWER, UPPER };

    Move     move; // uint16_t + int16_t
    int      eval;
    Bound    bound;
    uint16_t hash;
    uint16_t age;
    uint8_t  depth;
};

static_assert(sizeof(TranspositionTableEntry) == 16);

struct alignas(64) TranspositionTableBlock {
    std::array<TranspositionTableEntry, 4> entries{};
};

static_assert(sizeof(TranspositionTableBlock) == 64);

struct TranspositionTable {
    constexpr static std::size_t TT_SIZE{1ULL << 24};
    static_assert((TT_SIZE & (TT_SIZE - 1)) == 0);
    std::vector<TranspositionTableBlock> transposition_table{TT_SIZE};

    void
    add(const uint64_t index, const auto tt_entry) {
        assert(tt_entry.hash == static_cast<uint16_t>(index >> 48));
        auto & block = transposition_table[index & (transposition_table.size() - 1)].entries;

        for (auto & entry : block) {
            if (entry.age == 0 || entry.hash == tt_entry.hash || entry.age + 8 <= tt_entry.age) {
                entry = tt_entry;
                return;
            }
        }
    }

    void
    prefetch(const uint64_t key) const {
        __builtin_prefetch(&transposition_table[key & (TT_SIZE - 1)]);
    }

    const TranspositionTableEntry *
    get(const uint64_t index) const {
        const auto & block = transposition_table[index & (transposition_table.size() - 1)].entries;

        for (const auto & entry : block) {
            if (entry.age != 0 && entry.hash == static_cast<uint16_t>(index >> 48)) {
                return &entry;
            }
        }

        return nullptr;
    }
};

struct PVTable {
    std::array<std::array<Move, MAX_PLY>, MAX_PLY> moves{};
    std::array<int, MAX_PLY>                       length{};
};

struct SearchContext {
    Accumulators accumulator_stack;
    Killers      killers;
    History      history;
    PVTable      pv_table;
    std::size_t  nodes{0};
};

class ThreadPool {
public:
    explicit ThreadPool(std::size_t count = std::max<std::size_t>(
                            std::size_t{1}, std::thread::hardware_concurrency())) {
        assert(count > 0);
        threads_.reserve(count);

        std::generate_n(std::back_inserter(threads_), count, [this] {
            return std::thread{[this] {
                while (true) {
                    std::function<void()> task;
                    {
                        std::unique_lock lock{mx_};
                        cv_.wait(lock, [this] { return !tasks_.empty() || stop_; });

                        if (stop_ && tasks_.empty()) {
                            return;
                        }

                        task = std::move(tasks_.front());
                        tasks_.pop();
                    }

                    try {
                        task();
                    } catch (const std::exception & e) {
                        std::cerr << "Thread task threw: " << e.what() << std::endl;
                    } catch (...) {
                        std::cerr << "Thread task unknown error?" << std::endl;
                    }
                }
            }};
        });
    }

    ~ThreadPool() {
        {
            std::scoped_lock lock{mx_};
            stop_ = true;
        }

        cv_.notify_all();

        for (auto & thread : threads_) {
            if (thread.joinable()) {
                thread.join();
            }
        }
    }

    std::future<void>
    enqueue(std::function<void()> task) {
        auto packaged = std::make_shared<std::packaged_task<void()>>(std::move(task));
        auto future = packaged->get_future();

        {
            std::scoped_lock lock{mx_};
            if (stop_) {
                throw std::runtime_error{"Enqueue on already stopped ThreadPool"};
            }
            tasks_.emplace([packaged] { (*packaged)(); });
        }

        cv_.notify_one();
        return future;
    }

    ThreadPool(const ThreadPool &) = delete;
    ThreadPool &
    operator=(const ThreadPool &) = delete;
    ThreadPool(ThreadPool &&) = delete;
    ThreadPool &
    operator=(ThreadPool &&) = delete;

private:
    std::vector<std::thread>          threads_;
    std::queue<std::function<void()>> tasks_;
    std::mutex                        mx_;
    std::condition_variable           cv_;

    bool stop_{false};
};

class Segfault {
public:
    Segfault();

    int
    evaluateNetwork(const Board & board, const SearchContext & ctx);

    Move
    search(Board & board, std::size_t wtime, std::size_t btime, std::size_t winc, std::size_t binc,
           std::atomic<bool> & stop);

    Move
    search(Board & board, uint8_t depth, std::atomic<bool> & stop);

    int
    quiescence(Board & board, SearchContext & ctx, int alpha, int beta, uint8_t ply);

    int
    pvs(Board & board, SearchContext & ctx, int alpha, int beta, uint8_t depth, uint8_t ply,
        const bool null_move = false);

    void
    makeMoveAcc(Board & board, SearchContext & ctx, const Move move);

    void
    unmakeMoveAcc(Board & board, SearchContext & ctx, const Move move);

private:
    TranspositionTable transposition_table_;
    NetworkWeights     network_weights_;
    ThreadPool         thread_pool_;
    PVTable            pv_table_;

    Accumulators accumulator_stack_{};
    Killers      killers_{};
    History      history_{};

    std::chrono::time_point<std::chrono::system_clock> deadline_{
        std::chrono::system_clock::time_point::max()};
    std::atomic<bool> search_aborted_{false};
    std::size_t       nodes_{0};
};

} // namespace segfault
