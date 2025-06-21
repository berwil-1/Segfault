cmake_minimum_required(VERSION 3.16)
project(chess_bot VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(cmake/UpdateSubmodules.cmake)
include(CTest)
enable_testing()

# Add subdirectories
add_subdirectory(exts)
add_subdirectory(engine)
include_directories(engine)

# GoogleTest configuration
if(NOT TARGET gtest)
    # Assuming GoogleTest is added as a submodule in `exts/googletest`
    add_subdirectory(exts/googletest)
    enable_testing()
    include(GoogleTest)
endif()