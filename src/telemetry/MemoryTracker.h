#pragma once
#include <cstddef>

namespace telemetry {
    struct MemoryTracker {
        inline static thread_local std::size_t thread_allocations{0};
        inline static thread_local std::size_t thread_deallocations{0};

        static void record_allocation() noexcept { 
            thread_allocations++; 
        }
        
        static void record_deallocation() noexcept { 
            thread_deallocations++; 
        }
    };
}