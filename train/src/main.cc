#include "chess.hh"
#include "eval.hh"
#include "stockfish.hh"
#include "tiny_dnn/tiny_dnn.h"

#include <array>
#include <boost/process.hpp>
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
main() {
    /*using namespace std;
    namespace bp = boost::process;
    bp::ipstream is;
    bp::opstream os;

    bp::child c("./stockfish", bp::std_in<os, bp::std_out> is);
    os << "uci" << endl;
    os << "isready" << endl;
    os << "position fen rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1" << endl;
    os << "go depth 16" << endl;

    string line;
    string move_string;
    while (getline(is, line)) {
        if (!line.compare(0, 8, "bestmove")) {
            move_string = line;
            break;
        }
    }
    // Delete the "bestmove" part of the string and get rid of any trailing characters divided by
    // space
    move_string = move_string.substr(9, move_string.size() - 9);
    vector<string> mv;
    boost::split(mv, move_string, boost::is_any_of(" "));
    cout << "Stockfish move: " << mv.at(0) << endl;*/

    // Steam the PGN file
    auto file_stream = std::ifstream("./lichess_db_standard_rated_2013-01.pgn");
    // auto file_stream = std::ifstream("./my.pgn");
    // auto file_stream = std::ifstream("./broke.pgn");
    auto cnt = std::make_unique<MyCounter>();

    pgn::StreamParser parser_cnt(file_stream);
    auto              error = parser_cnt.readGames(*cnt);

    if (error) {
        std::cerr << "Error counter: " << error.message() << "\n";
        return 1;
    }

    const auto count = cnt.get()->getCount();
    std::cout << "Total count: " << count << "\n";
    auto vis = std::make_unique<MyVisitor>(count);

    file_stream.clear();
    file_stream.seekg(0, std::ios::beg);
    pgn::StreamParser parser_vis(file_stream);
    error = parser_vis.readGames(*vis);

    if (error) {
        std::cerr << "Error visitor: " << error.message() << "\n";
        return 1;
    }
}
