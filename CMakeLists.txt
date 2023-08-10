cmake_minimum_required(VERSION 3.10.1)

set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_INSTALL_PREFIX "install")

if(MSVC)
  add_definitions(-DUNICODE -D_UNICODE -D_CRT_SECURE_NO_WARNINGS=1)
  add_compile_options("/utf-8") # "/std:c++latest"
endif()

project(YJson VERSION 1.0 LANGUAGES CXX)

if(UNIX AND CMAKE_CXX_COMPILER_ID MATCHES "Clang" AND NOT ANDROID)
  add_compile_options(-stdlib=libc++)
  add_link_options(-stdlib=libc++)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fexperimental-library")  # std::format
endif()

include_directories(include)

link_directories (${PROJECT_SOURCE_DIR}/build)
add_library(yjson STATIC src/yjson.cpp)
add_executable(test test/test.cpp)
target_link_libraries(test PUBLIC yjson)
add_executable(usage test/usage.cpp)
target_link_libraries(usage PUBLIC yjson)
add_library(jslib SHARED test/jslib.cpp)
target_link_libraries(jslib PUBLIC yjson)

install(TARGETS yjson test usage
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
  RUNTIME DESTINATION bin
  PUBLIC_HEADER DESTINATION include
)
