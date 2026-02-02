cmake_minimum_required(VERSION 4.0)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(filetemp)

add_executable(filetemp)

target_sources(filetemp PRIVATE src/main.cpp
src/file_io.hpp)

target_include_directories(filetemp PRIVATE src)

add_subdirectory(vendor/argparse)

target_link_libraries(filetemp PRIVATE argparse)