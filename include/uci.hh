#pragma once

#include "chess.hh"

#include <atomic>
#include <cstddef>
#include <functional>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

namespace segfault {

using namespace chess;
using Callback = std::function<void(
    const std::string &, const std::vector<std::string> &, const std::size_t, const std::size_t)>;
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
    go(const std::string & command);

    void
    debug(const std::string & command);

    void
    quit();

    UciCommandHandler commands{
        {"uci", [this](const std::string &) { uci(); }},
        {"isready", [this](const std::string &) { isready(); }},
        {"ucinewgame", [this](const std::string &) { ucinewgame(); }},
        {"position", [this](const std::string & command) { position(command); }},
        {"go",
         [this](const std::string & command) {
             search_thread_ = std::thread([this, command]() { go(command); });
         }},
        {"debug", [this](const std::string & command) { debug(command); }},
        {"quit", [this](const std::string &) { quit(); }},
    };

    Board &                  board_;
    std::thread              search_thread_;
    std::atomic<bool>        search_done_;
    Callback                 callback_;
    std::vector<std::string> moves_;
    std::string              startpos_;
    bool                     active_;
};

} // namespace segfault
