cmake_minimum_required(VERSION 3.21)
project(sanic C)

set(CMAKE_C_STANDARD 17)

string(LENGTH "${CMAKE_SOURCE_DIR}/" SOURCE_PATH_SIZE)
add_definitions("-DSOURCE_PATH_SIZE=${SOURCE_PATH_SIZE}")

add_subdirectory(../ext/bdwgc "${CMAKE_CURRENT_BINARY_DIR}/ext/bdwgc")
include_directories(../ext/bdwgc/include)

add_library(sanic STATIC http_request.c http_response.c middleware.c http.c include/sanic.h http_header.c)
target_link_libraries(sanic PUBLIC gc)
