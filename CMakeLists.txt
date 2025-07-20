cmake_minimum_required(VERSION 3.13)
project(message_handler)
set(CMAKE_CXX_STANDARD 23)
set(CXX_STANDARD_REQUIRED YES)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_compile_options(
-Wall
-pedantic
-O2
)
add_subdirectory(src)
add_subdirectory(test)
include(FetchContent)
