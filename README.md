# Segfault

A small chess engine I made for the sake of it, since I've made a few bots already I will aim to make this one at least 2500 elo.

## Cutechess-Cli
./cutechess-cli -engine cmd=/home/william/Desktop/Programming/William/Chess/segfault/build/chess_bot proto=uci name=Segfault -engine cmd=/home/william/Desktop/Programming/William/Chess/stockfish/src/stockfish option.UCI_LimitStrength=true option.UCI_Elo=1350 proto=uci name=Stockfish -games 16 -concurrency 8 -repeat -each tc=60+1 -pgnout results.pgn

./cutechess-cli -engine cmd=/mnt/c/Users/CoolJWB/Desktop/Programming/Chess/segfault/build/chess_bot proto=uci name=Segfault -engine cmd=/mnt/c/Users/CoolJWB/Desktop/Programming/Chess/stockfish/src/stockfish option.UCI_LimitStrength=true option.UCI_Elo=1350 proto=uci name=Stockfish -games 16 -concurrency 16 -repeat -each tc=60+1 -pgnout results.pgn


## Evals (Stockfish 2k)

Documented (commit 4c55b660b3477995d1e83a2aa80e2710370b1e74)
Score of Segfault vs Stockfish: 31 - 89 - 8  [0.273] 128
...      Segfault playing White: 10 - 50 - 4  [0.188] 64
...      Segfault playing Black: 21 - 39 - 4  [0.359] 64
...      White vs Black: 49 - 71 - 8  [0.414] 128
Elo difference: -169.8 +/- 66.0, LOS: 0.0 %, DrawRatio: 6.3 %