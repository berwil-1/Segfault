#pragma once

#include "chess.hh"
#include "eval.hh"
#include "util.hh"

#include <array>
#include <boost/process.hpp>
#include <chrono>
#include <cstdint>
#include <fstream>
#include <ios>
#include <iostream>
#include <memory>
#include <ranges>
#include <rocksdb/db.h>
#include <span>
#include <sstream>
#include <string>
#include <unordered_set>

using namespace chess;
namespace bp = boost::process;

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
    MyVisitor(std::string name, std::size_t count) {
        total = count;
        logPath = std::string{"./fens-"} + name + ".progress";

        // Restore count from progress file if it exists
        {
            std::ifstream in(logPath);
            if (in.is_open()) {
                std::size_t restored;
                if (in >> restored) {
                    this->count = restored;
                    skipping = true;
                    std::cout << "Resuming from game " << this->count << "/"
                              << total << std::endl;
                }
            }
        }

        options.create_if_missing = true;
        rocksdb::DB::Open(options, std::string{"./fens-"} + name, &database);

        os << "uci" << std::endl;
        os << "isready" << std::endl;
    }

    virtual ~MyVisitor() {}

    void
    startPgn() {
        currentGame++;
        if (skipping && currentGame <= count) {
            skipThis = true;
        } else {
            skipping = false;
            skipThis = false;
            board = Board();
        }
    }

    void
    header(std::string_view key, std::string_view value) {}

    void
    startMoves() {
        if (skipThis)
            return;
        std::cout << count << "/" << total << std::endl;
    }

    void
    insertFen(const std::string & fen, const bool whiteToMove) {
        auto extract_score = [](const std::string & line,
                                const bool          whiteToMove) -> int {
            std::istringstream iss(line);
            std::string        token;

            while (iss >> token) {
                if (token == "cp") {
                    int cp;
                    iss >> cp;
                    return std::min(std::max(cp, -32000), 32000);
                } else if (token == "mate") {
                    int mate;
                    iss >> mate;
                    mate = 64000 - mate * 100;
                    return whiteToMove ? mate : -mate;
                }
            }
            throw std::runtime_error{"Unable to extract score..."};
        };

        os << "position fen " << fen << std::endl;
        os << "go depth 12" << std::endl;

        std::string line;
        std::string info;
        while (getline(is, line)) {
            if (line[0] == 'b') {
                break;
            }
            info = line;
        }

        auto score = extract_score(info, whiteToMove);
        score = whiteToMove ? score : -score;

        std::string existing;
        auto        s = database->Get(rocksdb::ReadOptions(), fen, &existing);
        if (s.ok()) {
            int avg = (std::stoi(existing) + score) / 2;
            batch.Put(fen, std::to_string(avg));
        } else {
            batch.Put(fen, std::to_string(score));
        }
    }

    void
    gatherBoards(Board & board, int depth, std::vector<Board> & boards) {
        if (depth == 0) {
            return;
        }

        Movelist movelist;
        movegen::legalmoves(movelist, board);

        for (const auto move : movelist) {
            board.makeMove(move);

            const auto fen = board.getFen();
            boards.emplace_back(fen);
            insertFen(fen, board.sideToMove() == Color::WHITE);
            gatherBoards(board, depth - 1, boards);

            board.unmakeMove(move);
        }
    }

    void
    move(std::string_view move, std::string_view comment) {
        if (skipThis)
            return;

        std::vector<Board> boards;
        auto               parsed = uci::parseSan(board, move);
        board.makeMove(parsed);

        const auto fen = board.getFen();
        insertFen(fen, board.sideToMove() == Color::WHITE);
        gatherBoards(board, 1, boards);

        database->Write(writeOpts, &batch);
        batch.Clear();
    }

    void
    endPgn() {
        if (skipThis)
            return;

        count++;
        persistCount();
    }

    auto
    getCount() -> std::size_t {
        return count;
    }

    auto
    getTotal() -> std::size_t {
        return total;
    }

private:
    void
    persistCount() {
        std::ofstream out(logPath, std::ios::trunc);
        out << count << std::endl;
        out.flush();
    }

    std::size_t count = 0;
    std::size_t total = 0;
    std::size_t currentGame = 0;
    bool        skipping = false;
    bool        skipThis = false;
    std::string logPath;
    Board       board;

    bp::ipstream is;
    bp::opstream os;
    bp::child    process{"./stockfish", bp::std_in<os, bp::std_out> is};

    rocksdb::DB *         database;
    rocksdb::Options      options;
    rocksdb::WriteOptions writeOpts;
    rocksdb::WriteBatch   batch;
};
