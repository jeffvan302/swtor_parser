cmake_minimum_required(VERSION 3.20)
project(swtor_parser_demo CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(swtor_parser STATIC
  swtor_parser.cpp
  swtor_parser.h
)

add_executable(demo main.cpp)
target_link_libraries(demo PRIVATE swtor_parser)

# Helpful flags
if (MSVC)
  target_compile_options(swtor_parser PRIVATE /W4 /utf-8)
  target_compile_options(demo PRIVATE /W4 /utf-8)
else()
  target_compile_options(swtor_parser PRIVATE -Wall -Wextra -Wpedantic)
  target_compile_options(demo PRIVATE -Wall -Wextra -Wpedantic)
endif()
