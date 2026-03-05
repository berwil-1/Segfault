#pragma once

#include "chess.hh"
#include "eval.hh"
#include "util.hh"

#include <array>
#include <chrono>
#include <cstdint>
#include <fstream>
#include <ios>
#include <iostream>
#include <memory>
#include <ranges>
#include <span>
#include <sstream>
#include <string>
#include <unordered_set>

using namespace chess;

class MyCounter : public pgn::Visitor {
public:
    virtual ~MyCounter() {}

    void
    startPgn() {
        count++;
    }

    void
    header(std::string_view key, std::string_view value) {}

    void
    startMoves() {}

    void
    move(std::string_view move, std::string_view comment) {}

    void
    endPgn() {}

    std::size_t
    getCount() {
        return count;
    }

private:
    std::size_t count;
    Board       board;
};

class MyVisitor : public pgn::Visitor {
public:
    MyVisitor(std::size_t count) {
        file.open("fens.txt", std::ios::app);
    }

    virtual ~MyVisitor() {
        std::cout << "Writing fens to file..." << std::endl;

        for (const auto & fen : fens) {
            file << fen << '\n';
        }
        file.close();

        std::cout << "Done." << std::endl;
    }

    void
    startPgn() {
        board = Board();
    }

    void
    header(std::string_view key, std::string_view value) {}

    void
    startMoves() {}

    void
    gatherBoards(Board & board, int depth, std::vector<Board> & boards) {
        if (depth == 0)
            return;

        Movelist movelist;
        movegen::legalmoves(movelist, board);

        for (const auto move : movelist) {
            board.makeMove(move);
            boards.emplace_back(board.getFen());
            gatherBoards(board, depth - 1, boards);
            board.unmakeMove(move);
        }
    }

    void
    move(std::string_view move, std::string_view comment) {
        auto parsed = uci::parseSan(board, move);
        board.makeMove(parsed);
        fens.emplace(board.getFen());

        /*std::vector<Board> boards;
        gatherBoards(board, 1, boards);
        for (const auto & brds : boards) {
            fens.emplace(brds.getFen());
        }*/
    }

    void
    endPgn() {}

private:
    std::ofstream                   file;
    Board                           board;
    std::unordered_set<std::string> fens;
};
