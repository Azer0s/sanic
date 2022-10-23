cmake_minimum_required(VERSION 3.21)
project(sanic C)
project(sanic_main C)

set(CMAKE_C_STANDARD 17)

add_definitions(-DSANIC_USE_CLANG_BLOCKS)
add_definitions(-DSANIC_LOG_DEBUG)
add_definitions(-DSANIC_LOG_TIME)
add_definitions(-DSANIC_SHOW_LOGO)
add_definitions(-DGC_DEBUG)

add_subdirectory(lib)
include_directories(lib/include)

add_executable(sanic_main main.c)
target_link_libraries(sanic_main gc)
target_link_libraries(sanic_main sanic)
