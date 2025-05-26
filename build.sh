#!/bin/bash
set -e

# Configure the build with clang
CC=clang CXX=clang++ cmake -B build

# Sync system clock with hardware clock (requires sudo)
sudo hwclock -s

# Build the project
cmake --build build