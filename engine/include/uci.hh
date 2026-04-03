#pragma once

#include "chess.hh"
#include "protocol.hh"
#include "segfault.hh"

#include <atomic>
#include <cstddef>
#include <functional>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

namespace segfault {

using namespace chess;
using UciCommand = std::function<void(const std::string &)>;
using UciCommandHandler = std::unordered_map<std::string, UciCommand>;

class Uci : Protocol {
public:
    explicit Uci(Segfault & segfault, Board & board) : segfault_(segfault), board_(board) {}

    void
    start();

    std::vector<std::string>
    getMoves();

    std::string
    getStartFen();

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
    go(const std::string & command, std::atomic<bool> & stop);

    void
    stop(const std::string & command);

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
             search_thread_ = std::thread([this, command](auto stop) { go(command, stop); },
                                          std::ref(search_stop_));
         }},
        {"stop", [this](const std::string & command) { search_stop_ = true; }},
        {"debug", [this](const std::string & command) { debug(command); }},
        {"quit", [this](const std::string &) { quit(); }},
    };

    Segfault &               segfault_;
    Board &                  board_;
    std::thread              search_thread_;
    std::atomic<bool>        search_done_;
    std::atomic<bool>        search_stop_;
    std::vector<std::string> moves_;
    std::string              startpos_;
    bool                     active_;
};

} // namespace segfault
