file(GLOB TEST_SRC CONFIGURE_DEPENDS *.cc)

add_executable(engine_tests ${TEST_SRC})
target_link_libraries(engine_tests PRIVATE engine gtest gtest_main)

# Correct include path pointing to the googletest headers
target_include_directories(engine_tests PRIVATE ${CMAKE_SOURCE_DIR}/exts/googletest/googletest/include)

include(GoogleTest)
gtest_discover_tests(engine_tests)
