cmake_minimum_required(VERSION 3.16)
project(chess_bot VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(include)

file(GLOB SRC_FILES CONFIGURE_DEPENDS src/*.cc)

add_executable(chess_bot ${SRC_FILES})
