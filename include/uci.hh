#pragma once

#include "chess.hh"

#include <functional>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

namespace segfault {

using namespace chess;
using Callback = std::function<void(const std::string &, const std::vector<std::string> &)>;
using UciCommand = std::function<void(const std::string &)>;
using UciCommandHandler = std::unordered_map<std::string, UciCommand>;

class Uci {
public:
    explicit Uci(Board & board) : board_(board) {}

    void
    start();

    std::vector<std::string>
    getMoves();

    std::string
    getStartFen();

    void
    setCallback(Callback func);

private:
    void
    uci();

    void
    isready();

    void
    ucinewgame();

    void
    position(const std::string & command);

    void
    go();

    void
    quit();

    UciCommandHandler commands{
        {"uci", [this](const std::string &) { uci(); }},
        {"isready", [this](const std::string &) { isready(); }},
        {"ucinewgame", [this](const std::string &) { ucinewgame(); }},
        {"position", [this](const std::string & command) { position(command); }},
        {"go", [this](const std::string &) { search_thread_ = std::thread([this]() { go(); }); }},
        {"quit", [this](const std::string &) { quit(); }},
    };

    Board &                  board_;
    std::thread              search_thread_;
    Callback                 callback_;
    std::vector<std::string> moves_;
    std::string              startpos_;
    bool                     active_;
};

} // namespace segfault
