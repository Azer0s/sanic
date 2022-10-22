cmake_minimum_required(VERSION 3.21)
project(uuid4 C)

set(CMAKE_C_STANDARD 17)

add_library(uuid4 STATIC uuid4/src/uuid4.c)
