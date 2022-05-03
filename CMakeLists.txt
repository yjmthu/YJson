cmake_minimum_required(VERSION 3.14)
cmake_policy(SET CMP0074 NEW)
set(CMAKE_CXX_STANDARD 11)

project(YJson)

if (MSVC)
    add_compile_options("/utf-8")
endif ()

add_library(yjson STATIC
    yjson.cpp
)