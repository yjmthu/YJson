cmake_minimum_required(VERSION 3.10)

set(CMAKE_CXX_STANDARD 20)
if (MSVC)
    add_definitions(-DUNICODE -D_UNICODE -D_CRT_SECURE_NO_WARNINGS=1)
    add_compile_options("/utf-8")
    # add_compile_options("/std:c++latest")
endif ()

project(YJson)

include_directories(include)

add_library(yjson STATIC src/yjson.cpp)
add_executable(test test/main.cpp)

target_link_libraries(test yjson)

