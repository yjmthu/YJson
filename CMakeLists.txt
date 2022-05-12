cmake_minimum_required(VERSION 3.14)
cmake_policy(SET CMP0074 NEW)
set(CMAKE_CXX_STANDARD 17)

# if (UNIX)
#     SET(CMAKE_C_COMPILER /usr/bin/clang)
#     SET(CMAKE_CXX_COMPILER /usr/bin/clang++)
# endif()

project(YJson)

SET(CMAKE_BUILD_TYPE "Debug")

set(PROJECT_BINARY_DIR ${CMAKE_CURRENT_SOURCE_DIR}/build)
message(${PROJECT_BINARY_DIR})
set(CMAKE_INCLUDE_CURRENT_DIR ON)

if (MSVC)
    add_compile_options("/utf-8")
endif ()

add_library(yjson STATIC
    yjson.cpp
)

target_compile_features(yjson PRIVATE cxx_std_17)

add_executable(test
    test/main.cpp
)

target_link_libraries(test
    yjson
)

