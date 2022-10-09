cmake_minimum_required(VERSION 3.21)
project(sanic C)
project(sanic_main C)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_COMPILER clang)

add_definitions(-DUSE_CLANG_BLOCKS)

add_library(sanic http.c http_request.c http_response.c http_header.h http_method.h log.h)
add_executable(sanic_main main.c)

target_link_libraries(sanic_main sanic)
