cmake_minimum_required(VERSION 3.20)
project(CoreAllocator CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -march=native -fno-exceptions -fno-rtti -Wall -Wextra -Werror")

add_executable(allocator_test
    src/core/MemoryBlock.cpp
    src/engine/FreeList.cpp
    src/bridges/GlobalOverride.cpp
)