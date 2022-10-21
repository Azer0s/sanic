cmake_minimum_required(VERSION 3.21)
project(sanic C)
project(sanic_main C)

set(CMAKE_C_STANDARD 17)

add_definitions(-DUSE_CLANG_BLOCKS)

add_subdirectory(lib)
include_directories(lib/include)

add_executable(sanic_main main.c)
target_link_libraries(sanic_main gc)
target_link_libraries(sanic_main sanic)
