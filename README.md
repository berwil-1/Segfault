# Segfault

A small chess engine I made for the sake of it, since I've made a few bots already I will aim to make this one at least 2500 elo.

[Event "?"]
[Site "?"]
[Date "2025.05.27"]
[Round "1"]
[White "Stockfish"]
[Black "Segfault"]
[Result "1-0"]
[ECO "A40"]
[GameDuration "00:01:50"]
[GameEndTime "2025-05-27T15:50:06.535 CEST"]
[GameStartTime "2025-05-27T15:48:15.827 CEST"]
[Opening "Queen's pawn"]
[PlyCount "73"]
[Termination "illegal move"]
[TimeControl "40/60"]

1. d4 {+0.34/25 6.4s} e6 2. c4 {+0.36/19 1.2s} Qh4 3. g3 {+1.44/15 0.81s} Qxh2
4. Rxh2 {+6.69/21 2.1s} Nc6 5. Bd2 {+6.92/20 2.9s} Nxd4 6. Bg2 {+6.52/21 4.9s}
Nf6 {0.61s} 7. Na3 {+6.21/20 1.8s} Ng4 {1.9s} 8. e3 {+6.32/20 1.3s} Nxh2 {3.2s}
9. b4 {+5.84/20 1.6s} Nf5 10. Qc2 {+5.32/23 5.8s} Bd6 {0.57s}
11. c5 {+4.88/19 1.2s} Be5 12. O-O-O {+4.74/19 1.5s} Ng4 {2.3s}
13. Nh3 {+5.09/21 3.0s} d5 {3.8s} 14. Bf3 {+4.37/19 3.0s} Nh2 {13s}
15. g4 {+4.70/20 3.9s} Nxf3 {13s} 16. Nb5 {+4.10/19 1.1s} Bd7 {2.5s}
17. c6 {+1.40/20 3.3s} bxc6 {1.4s} 18. Nc3 {+0.36/17 2.7s} Nh6 {0.64s}
19. e4 {-1.31/17 2.3s} Nxd2 {5.5s} 20. Qxd2 {-1.27/15 0.53s} Nxg4 {0.62s}
21. f3 {-0.92/18 1.8s} Nh2 22. Ng1 {+1.23/13 0.21s} Rb8 23. f4 {-0.65/14 1.5s}
Bxc3 24. Qf2 {-0.02/17 0.51s} Ng4 25. Qxa7 {-4.30/19 1.1s} Bb2+
26. Kd2 {-0.70/13 0.19s} Ke7 27. Qc5+ {-3.46/18 0.87s} Kf6
28. e5+ {-0.40/13 0.24s} Kf5 29. Qc2+ {+1.79/13 0.47s} Kxf4
30. Qxb2 {+1.49/15 0.24s} Nxe5 31. Qc3 {+M27/19 0.34s} Nc4+
32. Ke1 {+4.74/17 0.40s} f6 33. Kf2 {+3.06/12 0.046s} Ra8
34. Qg3+ {+3.68/9 0.015s} Ke4 35. Rd4+ {+M7/9 0.013s} Kxd4
36. Ne2+ {+M5/10 0.029s} Ke4
37. Qf3+ {+M3/26 0.019s, Black makes an illegal move: d4e4} 1-0

r6r/2pb2pp/2p1pp2/3p4/1Pn1k3/5Q2/P3NK2/8 b - - 3 37

## Cutechess-Cli
./cutechess-cli -engine cmd=/home/william/Desktop/Programming/William/Chess/segfault/build/chess_bot proto=uci name=Segfault -engine cmd=/home/william/Desktop/Programming/William/Chess/stockfish/src/stockfish option.UCI_LimitStrength=true option.UCI_Elo=1350 proto=uci name=Stockfish -games 10 -repeat -each tc=40/60 -pgnout results.pgn

./cutechess-cli -engine cmd=/mnt/c/Users/CoolJWB/Desktop/Programming/Chess/segfault/build/chess_bot proto=uci name=Segfault -engine cmd=/mnt/c/Users/CoolJWB/Desktop/Programming/Chess/stockfish/src/stockfish option.UCI_LimitStrength=true option.UCI_Elo=1350 proto=uci name=Stockfish -games 10 -repeat -each tc=40/60 -pgnout results.pgn
