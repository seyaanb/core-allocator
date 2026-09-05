cmake_minimum_required(VERSION 3.20)
project(CoreAllocator CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -march=native -fno-exceptions -fno-rtti -Wall -Wextra -Werror")

# Core Library Files
set(CORE_SOURCES 
    src/core/MemoryBlock.cpp
    src/engine/FreeList.cpp
    src/bridges/GlobalOverride.cpp
)

# 1. The Core Deliverable: A Static Library
add_library(core_allocator STATIC ${CORE_SOURCES})

# 2. The Correctness Test
add_executable(allocator_test tests/main.cpp)
target_link_libraries(allocator_test PRIVATE core_allocator)

# Disable benchmark's internal tests so it doesn't look for GTest
set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "Disable benchmark tests" FORCE)
set(BENCHMARK_ENABLE_GTEST_TESTS OFF CACHE BOOL "Disable GTest" FORCE)

include(FetchContent)
FetchContent_Declare(
    benchmark
    GIT_REPOSITORY https://github.com/google/benchmark.git
    GIT_TAG v1.8.3
)
FetchContent_MakeAvailable(benchmark)

# 3. The Benchmark Target
add_executable(allocator_bench benchmarks/AllocatorBench.cpp)
target_link_libraries(allocator_bench PRIVATE core_allocator benchmark::benchmark benchmark::benchmark_main pthread)