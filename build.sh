#!/bin/bash
set -e

# Configure the build with clang
CC=clang CXX=clang++ cmake -B build -DCMAKE_BUILD_TYPE=Release

# Sync system clock with hardware clock (requires sudo)
sudo hwclock -s

# Build the project
cmake --build build