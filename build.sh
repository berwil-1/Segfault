#!/bin/bash
set -e

# Define extra aggressive flags
# CXX_FLAGS="-O3 -march=native -flto -fno-omit-frame-pointer -fno-rtti"
CXX_FLAGS="-O3 -march=skylake -mtune=skylake -flto=auto -fno-omit-frame-pointer -fno-rtti -fvisibility=hidden"
LD_FLAGS="-flto=auto"

# Configure the build with Clang and optimizations
CC=gcc CXX=g++ cmake -B build -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_CXX_FLAGS_RELEASE="$CXX_FLAGS" \
    -DCMAKE_EXE_LINKER_FLAGS_RELEASE="$LD_FLAGS"

# Build the project
cmake --build build --parallel
