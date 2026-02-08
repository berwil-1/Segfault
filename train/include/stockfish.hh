#pragma once

#include "chess.hh"
#include "eval.hh"
#include "process.hh"

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
#include <fstream>

using namespace chess;

void
write_process_fen(const Process & proc, const std::string & fen) {
    write_to_process(proc.stdin_fd, "ucinewgame\n");
    write_to_process(proc.stdin_fd, "position fen " + fen + "\n");
    // write_to_process(proc.stdin_fd, "eval\n");
    write_to_process(proc.stdin_fd, "go depth 16\n");
    write_to_process(proc.stdin_fd, "ucinewgame\n");
}

std::string
write_process_uci(std::vector<Move> & moves) {
    std::string out = "";

    // Run stockfish
    auto proc = start_process(
        "/usr/bin/stdbuf",
        {"-o0", "/mnt/c/Users/CoolJWB/Desktop/Programming/chess/stockfish/src/stockfish"});
    write_to_process(proc.stdin_fd, "uci\n");
    write_to_process(proc.stdin_fd, "setoption name Threads value 16");

    write_to_process(proc.stdin_fd, "ucinewgame\n");
    //const auto span = std::span<Move>(moves.data(), moves.size());

    //std::cout << " Moves: [";
    auto total = std::size_t{0};

    for (auto count = std::size_t{1}; count <= moves.size(); count++) {
        //const auto  subspan = std::span(moves.data(), count);
        std::string subspan_moves = "";

        //for (auto move : subspan) {
        for (auto index = std::size_t{0}; index < count; index++) {
            subspan_moves += uci::moveToUci(moves[index]) + " ";
        }
        //std::cout << "subspan_moves: " << subspan_moves << std::endl;

        /*auto progress = (count * 100 / moves.size());
        if (progress >= total + 10) {
            std::cout << "o";
            total += 10; // step by 10 consistently
        }*/

        write_to_process(proc.stdin_fd, "position startpos moves " + subspan_moves + "\n");
        write_to_process(proc.stdin_fd, "go depth 16\n");

        if (count % 64 == 0) {
            write_to_process(proc.stdin_fd, "ucinewgame\n");
            write_to_process(proc.stdin_fd, "quit\n", true);
            out += read_from_process(proc.stdout_fd);

            proc = start_process(
                "/usr/bin/stdbuf",
                {"-o0", "/mnt/c/Users/CoolJWB/Desktop/Programming/chess/stockfish/src/stockfish"});
            write_to_process(proc.stdin_fd, "uci\n");
            write_to_process(proc.stdin_fd, "setoption name Threads value 16");
        }
    }
    //std::cout << "]" << std::endl;

    write_to_process(proc.stdin_fd, "ucinewgame\n");
    write_to_process(proc.stdin_fd, "quit\n", true);
    out += read_from_process(proc.stdout_fd);

    return out;
}

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
    MyVisitor(std::size_t count) : count_{count}, done_{0} {}

    virtual ~MyVisitor() {}

    void
    startPgn() {
        board_move = board;
        moves.clear();
        boards_move.clear();
    }

    void
    header(std::string_view key, std::string_view value) {}

    void
    startMoves() {}

    void
    move(std::string_view move, std::string_view comment) {
        auto parsed = uci::parseSan(board_move, move);
        moves.push_back(parsed);
        board_move.makeMove(parsed);
        boards_move.push_back(board_move);
    }

    void
    endPgn() {
        std::vector<float> evals;

        setvbuf(stdout, nullptr, _IONBF, 0);
        auto out = write_process_uci(moves);
        done_++;
        std::cout << "Progress: " << done_ << "/" << count_ << 
            " (" << moves.size() << ")" << std::endl;

        std::stringstream ss(out);
        std::string       prev;
        std::string       current;

        auto extract_score = [](const std::string & line) -> int {
            std::istringstream iss(line);
            std::string        token;

            while (iss >> token) {
                if (token == "cp") {
                    int cp;
                    iss >> cp;
                    return cp;
                } else if (token == "mate") {
                    int mate;
                    iss >> mate;
                    return mate > 0 ? 10000 - mate * 100 : -10000 + mate * 100;
                }
            }
            return 0;
        };

        std::ofstream file("./eval.txt", std::ios::app);
        bool          white = false;

        while (std::getline(ss, current, '\n')) {
            if (prev.rfind("info", 0) == 0 && current.rfind("bestmove", 0) == 0) {
                auto cp = extract_score(prev);
                //auto score = cp / 100.0f;
                auto score = cp;

                file << boards_move.at(evals.size()).getFen() << " ; " << (white ? cp : -cp)
                << '\n';
                evals.push_back(white ? score : -score);
                white = !white;
            }

            prev = current;
        }

        file.close();
    }

private:
    std::size_t count_;
    std::size_t done_;

    Board              board;
    Board              board_move;
    std::vector<Move>  moves;
    std::vector<Board> boards_move;
};