# Gather sources
file(GLOB_RECURSE ENGINE_SRC CONFIGURE_DEPENDS src/*.cc)
file(GLOB_RECURSE ENGINE_HDR CONFIGURE_DEPENDS include/*.hh)

# Create engine library
add_library(engine ${ENGINE_SRC} ${ENGINE_HDR})
target_include_directories(engine PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Create executable from engine
add_executable(chess_bot src/main.cc)
target_link_libraries(chess_bot PRIVATE engine)

# Add tests
add_subdirectory(tests)
