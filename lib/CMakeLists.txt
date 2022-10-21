cmake_minimum_required(VERSION 3.21)
project(sanic C)

set(CMAKE_C_STANDARD 17)

add_subdirectory(../ext/bdwgc "${CMAKE_CURRENT_BINARY_DIR}/ext/bdwgc")
add_library(sanic http.c http_request.c http_response.c middleware.c)
include_directories(../ext/bdwgc/include)
target_link_libraries(sanic gc)
