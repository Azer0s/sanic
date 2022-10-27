cmake_minimum_required(VERSION 3.21)
project(sanic C)

set(CMAKE_C_STANDARD 17)

string(LENGTH "${CMAKE_SOURCE_DIR}/" SOURCE_PATH_SIZE)
add_definitions("-DSOURCE_PATH_SIZE=${SOURCE_PATH_SIZE}")

add_definitions(-DGC_THREADS)
add_definitions(-DPARALLEL_MARK)
add_definitions(-DTHREAD_LOCAL_ALLOC)
set(enable_threads ON)

add_subdirectory(../ext/bdwgc "${CMAKE_CURRENT_BINARY_DIR}/ext/bdwgc")
include_directories(../ext/bdwgc/include)

add_subdirectory(../ext/ "${CMAKE_CURRENT_BINARY_DIR}/ext/uuid4")
include_directories(../ext/uuid4/src)

add_definitions(-Wno-string-plus-int)
add_definitions(-Wno-format-extra-args)

if (UNIX)
    set(server_impl server_bsd.c)
elseif (LINUX)
    message( FATAL_ERROR "linux support for sanic is not done yet :/" )
    set(server_impl server_linux.c.c)
else()
    message( FATAL_ERROR "sanic is not supported on this system :(" )
endif()

add_library(sanic STATIC
        http_method.c
        http_request.c
        http_response.c
        middleware.c
        route.c
        sanic.c
        ${server_impl}

        internal/http_util.c
        internal/http_param.c
        internal/request_handler.c
        internal/request_util.c
        internal/middleware_handler.c)
target_link_libraries(sanic PUBLIC gc)
target_link_libraries(sanic PUBLIC uuid4)
