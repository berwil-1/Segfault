set(LIBTORCH_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include/libtorch)

# Gather sources
file(GLOB_RECURSE ENGINE_SRC CONFIGURE_DEPENDS src/*.cc src/*.c)
file(GLOB_RECURSE ENGINE_HDR CONFIGURE_DEPENDS include/*.hh include/*.h)

set(CMAKE_CUDA_COMPILER_FORCED TRUE)
find_package(Torch REQUIRED PATHS ${LIBTORCH_DIR})
if(NOT TARGET Torch::Torch)
  add_library(Torch::Torch INTERFACE IMPORTED)
  target_include_directories(Torch::Torch INTERFACE "${TORCH_INCLUDE_DIRS}")
  target_link_libraries(Torch::Torch INTERFACE "${TORCH_LIBRARIES}")
endif()

# Create engine library
add_library(engine ${ENGINE_SRC} ${ENGINE_HDR})
target_include_directories(engine PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(engine PUBLIC
    Torch::Torch)

# Create executable from engine
add_executable(chess_bot src/main.cc)
target_link_libraries(chess_bot PRIVATE engine)
