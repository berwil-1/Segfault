# Segfault

A small chess engine I made for the sake of it. I've made a few bots already, so this time the goal is to push past 2500 Elo. Let's see how it goes, maybe I'll learn something along the way.

## Features

- **Search** - Principal Variation Search with aspiration windows, iterative deepening, and deadline-based time management.
- **Pruning / reductions** - null move pruning, Late Move Reductions, killer move heuristic.
- **Quiescence search** - captures, checks, and promotions; full move generation when in check.
- **Transposition table** with a triangular PV table for line reconstruction.
- **Evaluation** - custom neural network (258 > 512 > 512 > 256 > 1), run via matmul using AVX2 FMA intrinsics.
- **UCI protocol** - basic functionality, uci, isready, position fen, go depth, stop, etc.

## Building

### Requirements

- CMake
- GCC 15.2 (C++20)
- A CPU with AVX2

### Build

```sh
./build.sh
```

Binary lands in `build/engine/chess_bot`. Run it with:

```sh
./build/engine/chess_bot
```

## Architecture

- [chess-library](https://github.com/Disservin/chess-library) handles board representation, move generation, and make/unmake.
- Search runs make/unmake on a single board with a small undo stack (Zobrist hash, castling rights, en passant)
- NN weights are extracted from a PyTorch-trained model into fixed-size `std::array` structs and multiplied with AVX2 (`_mm256_fmadd_ps`). Additionally quantized as a post-process step with a Python script.

## Dependencies

**To build the engine:**
- None

**To do NN training:**
- libtorch - trains the evaluation network.
- RocksDB - stores training positions.
- Boost - process management for spawning Stockfish.
- Stockfish - ground-truth eval for training positions.

## Known issues

Used to segfault quite often, should be fine now...
