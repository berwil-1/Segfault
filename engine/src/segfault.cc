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
Segfault::search(Board & board, std::size_t wtime, std::size_t btime) {
    Movelist moves;
    generateAllMoves(moves, board);
    std::priority_queue<std::pair<int, int>> queue;

    const auto start = std::chrono::system_clock::now();
    auto       d = 6;

    while (moves.size() > 1) {
        for (auto & move : moves) {
            board.makeMove(move);
            const auto score = -negaAlphaBeta(board, -INT32_MAX, INT32_MAX, d);
            board.unmakeMove(move);
            queue.emplace(score, move.move());
            std::cout << uci::moveToUci(move) << ": " << score << std::endl;
        }

        // make sure the top element doesn't have any close competitors
        const auto top = queue.top();
        queue.pop();
        moves.clear();
        moves.add(top.second);
        while (queue.top().first == top.first) {
            moves.add(queue.top().second);
            queue.pop();
        }
        d++;
    }

    auto end = std::chrono::system_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    std::cout << elapsed.count() << "ms\n" << std::endl;

    return *moves.begin();
}

} // namespace segfault
