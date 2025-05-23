#pragma once

#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

namespace segfault {

using UciCommand = std::function<void(const std::string &)>;
using UciCommandHandler = std::unordered_map<std::string, UciCommand>;

class Uci {
public:
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
    go();

    void
    quit();

    UciCommandHandler commands{
        {"uci", [this](const std::string &) { uci(); }},
        {"isready", [this](const std::string &) { isready(); }},
        {"ucinewgame", [this](const std::string &) { ucinewgame(); }},
        {"position", [this](const std::string & command) { position(command); }},
        {"go", [this](const std::string &) { go(); }},
        {"quit", [this](const std::string &) { quit(); }},
    };

    std::vector<std::string> moves;
    std::string              startpos;
    bool                     active;
};

} // namespace segfault
