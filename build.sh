#!/bin/bash
set -e

CXX_FLAGS="-Ofast -march=native -fopenmp"
LD_FLAGS="-flto -fopenmp"

# Erase old build
rm -rf build

# Configure the build with Clang and optimizations
CC=gcc-14 CXX=g++-14 cmake -B build -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_CUDA_HOST_COMPILER=g++-14 \
    -DCMAKE_CXX_FLAGS_RELEASE="$CXX_FLAGS" \
    -DCMAKE_EXE_LINKER_FLAGS_RELEASE="$LD_FLAGS" \
    -DCMAKE_EXPORT_COMPILE_COMMANDS=ON

# Rebuild the project
cmake --build build --parallel
