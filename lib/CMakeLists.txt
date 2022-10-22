cmake_minimum_required(VERSION 3.21)
project(sanic C)

set(CMAKE_C_STANDARD 17)

string(LENGTH "${CMAKE_SOURCE_DIR}/" SOURCE_PATH_SIZE)
add_definitions("-DSOURCE_PATH_SIZE=${SOURCE_PATH_SIZE}")

add_subdirectory(../ext/bdwgc "${CMAKE_CURRENT_BINARY_DIR}/ext/bdwgc")
include_directories(../ext/bdwgc/include)

add_compile_options(-Wno-string-plus-int)

add_library(sanic STATIC
        http_header.c
        http_request.c
        internal/http_util.c
        middleware.c
        internal/request_handler.c
        route.c
        internal/request_util.c
        server.c include/internal/http_util.h internal/middleware_handler.c include/internal/middleware_handler.h)
target_link_libraries(sanic PUBLIC gc)
