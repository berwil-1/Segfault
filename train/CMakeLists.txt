# Gather sources
file(GLOB_RECURSE TRAIN_SRC CONFIGURE_DEPENDS src/*.cc)
file(GLOB_RECURSE TRAIN_HDR CONFIGURE_DEPENDS include/*.hh)

# Create train library
add_library(train ${TRAIN_SRC} ${TRAIN_HDR})
target_include_directories(train PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Create executable from train
add_executable(train_bot src/main.cc)
target_link_libraries(train_bot PRIVATE train)