# Segfault
A small chess engine I made for the sake of it, since I've made a few bots already I will aim to make this one at least 2500 elo. Let's see how it goes, maybe I'll learn something along the way.

## Cutechess-Cli
../cutechess/build/cutechess-cli -engine cmd=/home/cooljwb/segfault/build/engine/chess_bot dir=/home/cooljwb/segfault/ proto=uci name=Segfault -engine cmd=/home/cooljwb/segfault/stockfish option.UCI_LimitStrength=true option.UCI_Elo=1500 proto=uci name=Stockfish -games 32 -concurrency 32 -repeat -each tc=300+1 -pgnout results.pgn

## Evals (Stockfish)



## Known issues
Used to segfault quite often, should be fine now...
