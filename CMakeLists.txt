cmake_minimum_required(VERSION 4.0)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(filetemp)

add_executable(filetemp)

target_sources(filetemp PRIVATE src/main.cpp
src/file_io.hpp
src/log.hpp
src/cmake_gen.h
src/cmake_gen.cpp)

target_include_directories(filetemp PRIVATE src)

set(SPDLOG_USE_STD_FORMAT ON)

add_subdirectory(vendor/argparse)
add_subdirectory(vendor/spdlog)

target_link_libraries(filetemp PRIVATE argparse spdlog)
target_compile_definitions(filetemp PRIVATE $<$<CONFIG:Debug>:FT_DEBUG>)