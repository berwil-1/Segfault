#include "chess.hh"
#include "eval.hh"
#include "stockfish.hh"
#include "tiny_dnn/tiny_dnn.h"

#include <array>
#include <chrono>
#include <cstdint>
#include <ios>
#include <iostream>
#include <memory>
#include <ranges>
#include <span>
#include <sstream>
#include <string>
#include <unordered_set>

using namespace chess;
using namespace tiny_dnn;
using namespace segfault;

using PgnType = std::vector<Move>;
using PgnList = std::vector<PgnType>;

constexpr auto board_size = 256;

int
main(int argc, char ** argv) {
    if (argc < 2) {
        std::cout << "Invalid amount of arguments..." << std::endl;
        return 1;
    }

    // Steam the PGN file
    auto file_stream = std::ifstream(argv[1]);
    if (!file_stream) {
        std::cout << "Failed to open file..." << std::endl;
    }

    auto              cnt = std::make_unique<MyCounter>();
    pgn::StreamParser parser_cnt(file_stream);
    auto              error = parser_cnt.readGames(*cnt);

    if (error) {
        std::cerr << "Error counter: " << error.message() << "\n";
        return 1;
    }

    const auto count = cnt.get()->getCount();
    std::cout << "Total count: " << count << "\n";
    auto vis = std::make_unique<MyVisitor>(argv[1], count);

    file_stream.clear();
    file_stream.seekg(0, std::ios::beg);
    pgn::StreamParser parser_vis(file_stream);
    error = parser_vis.readGames(*vis);

    if (error) {
        std::cerr << "Error visitor: " << error.message() << "\n";
        return 1;
    }
}
