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
src/cmake_gen.cpp
src/gen.h
src/gen.cpp
src/file_types.h)
add_subdirectory(src/arg)

target_include_directories(filetemp PRIVATE src)

set(SPDLOG_USE_STD_FORMAT ON)

add_subdirectory(vendor/argparse)
add_subdirectory(vendor/spdlog)
add_subdirectory(vendor/yaml-cpp)

target_link_libraries(filetemp PRIVATE argparse spdlog yaml-cpp)
target_compile_definitions(filetemp PRIVATE $<$<CONFIG:Debug>:FT_DEBUG>)

if(MSVC)
    target_compile_options(filetemp PRIVATE /utf-8)
endif()

if(WIN32)
    target_compile_definitions(filetemp PRIVATE FT_PLATFORM_WINDOWS)
elseif(UNIX)
    target_compile_definitions(filetemp PRIVATE FT_PLATFORM_UNIX)
else()
    message(FATAL "System not supported.")
endif()

target_compile_options(filetemp PRIVATE
    $<$<CXX_COMPILER_ID:GNU>:-Wall -Wextra -pedantic -Werror>
    $<$<CXX_COMPILER_ID:Clang>:-Wall -Wextra -pedantic -Werror>
    $<$<CXX_COMPILER_ID:MSVC>:/W4 /WX>
)