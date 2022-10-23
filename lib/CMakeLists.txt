cmake_minimum_required(VERSION 3.21)
project(sanic C)

set(CMAKE_C_STANDARD 17)

string(LENGTH "${CMAKE_SOURCE_DIR}/" SOURCE_PATH_SIZE)
add_definitions("-DSOURCE_PATH_SIZE=${SOURCE_PATH_SIZE}")

add_definitions(-DGC_THREADS)

add_subdirectory(../ext/bdwgc "${CMAKE_CURRENT_BINARY_DIR}/ext/bdwgc")
include_directories(../ext/bdwgc/include)

add_subdirectory(../ext/ "${CMAKE_CURRENT_BINARY_DIR}/ext/uuid4")
include_directories(../ext/uuid4/src)

add_definitions(-Wno-string-plus-int)
add_definitions(-Wno-format-extra-args)

add_library(sanic STATIC
        http_method.c
        http_request.c
        http_response.c
        middleware.c
        route.c
        sanic.c
        server.c

        internal/http_util.c
        internal/http_param.c
        internal/request_handler.c
        internal/request_util.c
        internal/middleware_handler.c)
target_link_libraries(sanic PUBLIC gc)
target_link_libraries(sanic PUBLIC uuid4)
