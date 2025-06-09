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

Transposition table move (commit 95c79e22139538b7c2b36f01725c4c49a77450aa)
Score of Segfault vs Stockfish: 30 - 93 - 5  [0.254] 128
...      Segfault playing White: 10 - 53 - 1  [0.164] 64
...      Segfault playing Black: 20 - 40 - 4  [0.344] 64
...      White vs Black: 50 - 73 - 5  [0.410] 128
Elo difference: -187.2 +/- 68.8, LOS: 0.0 %, DrawRatio: 3.9 %
SPRT: llr 0 (0.0%), lbound -inf, ubound inf

## Known issues
uci
id name segfault
id author William Bergh
uciok
position fen 8/8/8/3k4/8/8/4K3/R7 b - - 96 133
go
terminate called after throwing an instance of 'std::out_of_range'
  what():  vector::_M_range_check: __n (which is 64) >= this->size() (which is 64)
Aborted (core dumped)