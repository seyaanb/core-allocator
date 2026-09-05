cmake_minimum_required(VERSION 3.20)
project(CoreAllocator CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -march=native -fno-exceptions -fno-rtti -Wall -Wextra -Werror")

# 1. Build the actual Library
add_library(core_allocator STATIC
    src/core/MemoryBlock.cpp
    src/engine/FreeList.cpp
    src/bridges/GlobalOverride.cpp
)

# 2. Add include directories so consumers can find headers
target_include_directories(core_allocator PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)

# 3. Build the Test Harness (Optional)
add_executable(allocator_test tests/main.cpp)

# Link the test harness to library
target_link_libraries(allocator_test PRIVATE core_allocator)