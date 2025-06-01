# Segfault

A small chess engine I made for the sake of it, since I've made a few bots already I will aim to make this one at least 2500 elo.

## Cutechess-Cli
./cutechess-cli -engine cmd=/home/william/Desktop/Programming/William/Chess/segfault/build/chess_bot proto=uci name=Segfault -engine cmd=/home/william/Desktop/Programming/William/Chess/stockfish/src/stockfish option.UCI_LimitStrength=true option.UCI_Elo=1350 proto=uci name=Stockfish -games 16 -concurrency 8 -repeat -each tc=60+1 -pgnout results.pgn

./cutechess-cli -engine cmd=/mnt/c/Users/CoolJWB/Desktop/Programming/Chess/segfault/build/chess_bot proto=uci name=Segfault -engine cmd=/mnt/c/Users/CoolJWB/Desktop/Programming/Chess/stockfish/src/stockfish option.UCI_LimitStrength=true option.UCI_Elo=1350 proto=uci name=Stockfish -games 16 -concurrency 16 -repeat -each tc=60+1 -pgnout results.pgn


