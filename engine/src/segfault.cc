#include "segfault.hh"

#include "eval.hh"
#include "search.hh"
#include "util.hh"

#include <chrono>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>
#include <array>

namespace {
using chess::Board;
using chess::Color;
using chess::Movelist;

auto
time_allocated_func(const Board & board, const Movelist & moves, const std::size_t wtime,
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
    
Segfault::Segfault() {
    if (torch::cuda::is_available())
        device = torch::kCUDA; // optional

    // 1) Load model weights
    load_module(*model, "model_best.pt");
    model->to(device);
    model->eval();

    std::cout << "Loaded model_best.pt\n";
}

Move
Segfault::search(Board & board, std::size_t wtime, std::size_t btime) {
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
    constexpr auto depth_min = 1;

    // std::cout << "fen: " << board.getFen() << "\n";

    for (auto d = depth_min; d < depth_max; d++) {
        /*std::cout << "info "
                  << "depth " << d << " score cp " << evaluateStockfish(board) << " time "
                  << std::chrono::duration_cast<std::chrono::milliseconds>(
                         std::chrono::system_clock::now() - start)
                         .count()
                  << std::endl;*/

        for (auto & eval : evals) {
            if (std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::system_clock::now() - start)
                    .count() > time_allocated) {
                break;
            }

            board.makeMove(eval.first);
            //const auto score = -negaAlphaBeta(board, -INT32_MAX, INT32_MAX, d);

            const auto enc = encode_board(board);

            // shape: [1, board_size]
            auto x = torch::from_blob((void *)enc.data(), {1, board_size},
                                        torch::TensorOptions().dtype(torch::kFloat32))
                            .clone()
                            .to(device);

            torch::NoGradGuard no_grad;
            auto               y = model->forward(x); // [1, 1]
            float              pred = y.item<float>(); // roughly in [-1, 1] given your training targets

            // Optional: convert back to centipawns with the same scale you used in training
            //float cp_est = pred * 1200.0f;
            constexpr auto k = 0.00368208f;
            auto score = static_cast<int>(std::log((1 / pred) - 1) / -k);
            
            board.unmakeMove(eval.first);
            //std::cout << uci::moveToUci(Move{eval.first}) << ": " << score << std::endl;
            eval.second = board.sideToMove() == Color::WHITE ? score : -score;

            // std::cout << Move{eval.first} << ": " << score << std::endl;
        }

        std::sort(evals.begin(), evals.end(),
                  [](const auto & a, const auto & b) { return a.second > b.second; });
        std::cout << "info "
                  << "depth " << d << " score cp " << evals.front().second << " time "
                  << std::chrono::duration_cast<std::chrono::milliseconds>(
                         std::chrono::system_clock::now() - start)
                         .count()
                  << std::endl;

        if (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() -
                                                                  start)
                .count() > time_allocated) {
            break;
        }
    }

    return evals.front().first;
}

std::array<float, board_size>
encode_board(const Board & board) {
    std::array<float, board_size> input{};

    constexpr auto pieces = std::array<float, 12>{1.0f,  3.0f,  3.25f,  5.0f,  9.0f,  100.0f,
                                                  -1.0f, -3.0f, -3.25f, -5.0f, -9.0f, -100.0f};

    auto indices = board.occ();

    while (!indices.empty()) {
        const auto index = indices.msb();
        const auto piece = board.at(index);
        const auto piece_value = 1.0f / (1.0f + std::exp(-0.06f * 2.0f * pieces[static_cast<int>(piece)]));
        const auto psqt_bonus = 1.0f / (1.0f + std::exp(-0.02f * piece_square_table_bonus(board, index, piece.color(), true)));

        const auto do_mobility =
            piece.type() == PieceType::QUEEN || piece.type() == PieceType::ROOK ||
            piece.type() == PieceType::BISHOP || piece.type() == PieceType::KNIGHT;
        const auto mobility = do_mobility ? (1.0f / (1.0f + std::exp(-0.05f * 
            mobility_bonus(board, index, piece.color(), true)))) : 0.0f;

        input[index] = piece_value;
        input[64 + index] = mobility;
        input[128 + index] = psqt_bonus;
        input[192 + index] = board.sideToMove() == chess::Color::WHITE ? 1 : -1;
        input[256 + index] = 1.0f / (1.0f + std::exp(-0.1f * (board.fullMoveNumber() - 50)));

        indices.clear(index);
    }

    return input;
}

} // namespace segfault
