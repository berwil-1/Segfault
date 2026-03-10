set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(LIBTORCH_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include/libtorch)

# Gather sources
file(GLOB_RECURSE TRAIN_SRC CONFIGURE_DEPENDS src/*.cc src/*.c)
file(GLOB_RECURSE TRAIN_HDR CONFIGURE_DEPENDS include/*.hh include/*.h)

find_package(Torch REQUIRED PATHS ${LIBTORCH_DIR})
if(NOT TARGET Torch::Torch)
  add_library(Torch::Torch INTERFACE IMPORTED)
  target_include_directories(Torch::Torch INTERFACE "${TORCH_INCLUDE_DIRS}")
  target_link_libraries(Torch::Torch INTERFACE "${TORCH_LIBRARIES}")
endif()

# Library
add_library(train ${TRAIN_SRC} ${TRAIN_HDR})

target_include_directories(train
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

find_package(RocksDB REQUIRED)

target_link_libraries(train
  PUBLIC
    Torch::Torch
    RocksDB::rocksdb
)

target_compile_options(train PRIVATE
  $<$<CXX_COMPILER_ID:Clang>:-Wno-deprecated-enum-enum-conversion>
)

# Executable
add_executable(train_bot src/main.cc)
target_link_libraries(train_bot PRIVATE train)
