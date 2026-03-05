set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Gather sources
file(GLOB_RECURSE TRAIN_SRC src/*.cc src/*.c)
file(GLOB_RECURSE TRAIN_HDR include/*.hh include/*.h)

# Library
add_library(train ${TRAIN_SRC} ${TRAIN_HDR})

# Your headers (PUBLIC so train_bot inherits)
target_include_directories(train
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

# Third-party headers (warnings off inside those)
# target_include_directories(train SYSTEM PRIVATE ${EIGEN3_INCLUDE_DIRS})

include(FetchContent)
FetchContent_Declare(
    Boost
    URL https://github.com/boostorg/boost/releases/download/boost-1.87.0/boost-1.87.0-cmake.tar.gz
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
set(BOOST_INCLUDE_LIBRARIES process)
FetchContent_MakeAvailable(Boost)

target_link_libraries(train PUBLIC Boost::process)

target_compile_options(train PRIVATE
  $<$<CXX_COMPILER_ID:Clang>:-Wno-deprecated-enum-enum-conversion>
)

# Executable
add_executable(train_bot src/main.cc)
target_link_libraries(train_bot PRIVATE train)
