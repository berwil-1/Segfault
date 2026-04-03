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
    const auto start = std::chrono::system_clock::now();
    auto       queue = std::priority_queue<std::pair<int, int>>{};

    Movelist moves;
    generateAllMoves(board, moves);

    for (auto d = 1; d <= 4; d++) {
        queue = {};
        std::cout << "d: " << d << std::endl;

        for (auto & move : moves) {
            board.makeMove(move);
            const auto score = -pvs(board, -INT32_MAX, INT32_MAX, d);
            board.unmakeMove(move);
            queue.emplace(score, move.move());
            std::cout << uci::moveToUci(move) << ": " << score << std::endl;
            if (stop)
                break;
        }

        // TODO:
        // const auto top = queue.top();
        // queue.pop();
        // while(queue.top() == top)
        // queue.pop();

        std::cout << "Move: " << uci::moveToUci(queue.top().second) << std::endl;

        if (stop)
            break;
    }

    auto end = std::chrono::system_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    std::cout << elapsed.count() << "ms\n" << std::endl;

    return queue.top().second;
}

} // namespace segfault
