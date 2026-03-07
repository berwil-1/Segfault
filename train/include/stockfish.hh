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
#include <rocksdb/db.h>
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
        total = count;
        options.create_if_missing = true;
        writeOpts.disableWAL = true; // huge write throughput gain
        rocksdb::DB::Open(options, "./fendb", &database);
    }

    virtual ~MyVisitor() {}

    void
    startPgn() {
        board = Board();
    }

    void
    header(std::string_view key, std::string_view value) {}

    void
    startMoves() {
        std::cout << count << "/" << total << std::endl;
    }

    void
    insertFen(const std::string & fen) {
        batch.Put(fen, {});

        if (batch.Count() >= 4096) {
            database->Write(writeOpts, &batch);
            batch.Clear();
        }
    }

    void
    gatherBoards(Board & board, int depth, std::vector<Board> & boards) {
        if (depth == 0)
            return;

        Movelist movelist;
        movegen::legalmoves(movelist, board);

        for (const auto move : movelist) {
            board.makeMove(move);

            const auto fen = board.getFen();
            boards.emplace_back(fen);
            insertFen(fen);
            gatherBoards(board, depth - 1, boards);

            board.unmakeMove(move);
        }
    }

    void
    move(std::string_view move, std::string_view comment) {
        std::vector<Board> boards;
        auto               parsed = uci::parseSan(board, move);
        board.makeMove(parsed);

        const auto fen = board.getFen();
        insertFen(fen);

        gatherBoards(board, 1, boards);
    }

    void
    endPgn() {
        count++;
    }

private:
    std::size_t           count;
    std::size_t           total;
    Board                 board;
    rocksdb::DB *         database;
    rocksdb::Options      options;
    rocksdb::WriteOptions writeOpts;
    rocksdb::WriteBatch   batch;
};
