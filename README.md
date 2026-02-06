# filetemp

A simple tool that generates minimal file templates.

**It only supports CMake for now.**

## Build

Use CMake to build filetemp:

```cmake
cd <filetemp_path>
cmake -B build -S .
cmake --build build
```

## Usage

Execute `filetemp --help` to discover its usage.

Here is an example of using filetemp:

```
filetemp cmake --version 4.0 --cstd 99 --cxxstd 20 --project myproject
```

It generates a CMakeLists.txt like this:

```cmake
cmake_minimum_required(VERSION 4.0)

set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 20)
project(myproject)

add_executable(myproject)
target_sources(myproject PRIVATE src/main.cpp)
target_include_directories(myproject PRIVATE src)
```

