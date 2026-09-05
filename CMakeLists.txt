cmake_minimum_required(VERSION 3.20)
project(CoreAllocator CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Note: Removed -fsanitize=thread. Sanitizers destroy benchmark latency accuracy.
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -march=native -fno-exceptions -fno-rtti -Wall -Wextra -Werror")

# Core Library Files
set(CORE_SOURCES 
    src/core/MemoryBlock.cpp
    src/engine/FreeList.cpp
    src/bridges/GlobalOverride.cpp
)

add_library(core_allocator STATIC
    src/core/MemoryBlock.cpp
    src/engine/FreeList.cpp
    src/bridges/GlobalOverride.cpp
)

add_executable(allocator_test tests/main.cpp)
target_link_libraries(allocator_test PRIVATE core_allocator)

# Fetch Google Benchmark
include(FetchContent)
FetchContent_Declare(
    benchmark
    GIT_REPOSITORY https://github.com/google/benchmark.git
    GIT_TAG v1.8.3
)
FetchContent_MakeAvailable(benchmark)

# Benchmark Target
add_executable(allocator_bench benchmarks/AllocatorBench.cpp ${CORE_SOURCES})
target_link_libraries(allocator_bench PRIVATE benchmark::benchmark benchmark::benchmark_main pthread)