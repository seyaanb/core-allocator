#pragma once
#include <cstddef>
#include <atomic>
#include <array>
#include <cstdio>
#include <cstdlib>

namespace telemetry {

struct GlobalMetrics {
    static inline std::array<std::atomic<size_t>, 5> allocs{};
    static inline std::array<std::atomic<size_t>, 5> deallocs{};
    static inline std::atomic<size_t> fallback_mallocs{0};
    
    static void print_report() noexcept {
        printf("\n=== TLS Allocator Telemetry Report ===\n");
        constexpr size_t sizes[] = {64, 128, 256, 512, 1024};
        for(int i = 0; i < 5; ++i) {
            printf("Size Class %4zuB : %zu Allocs | %zu Frees\n", 
                sizes[i], allocs[i].load(std::memory_order_relaxed), deallocs[i].load(std::memory_order_relaxed));
        }
        printf("Fallback std::malloc (>1024B): %zu\n", fallback_mallocs.load(std::memory_order_relaxed));
        printf("======================================\n\n");
    }
};

struct ThreadMetrics {
    size_t allocs[5]{0};
    size_t deallocs[5]{0};
    size_t fallback_mallocs{0};

    ~ThreadMetrics() {
        for(int i = 0; i < 5; ++i) {
            GlobalMetrics::allocs[i].fetch_add(allocs[i], std::memory_order_relaxed);
            GlobalMetrics::deallocs[i].fetch_add(deallocs[i], std::memory_order_relaxed);
        }
        GlobalMetrics::fallback_mallocs.fetch_add(fallback_mallocs, std::memory_order_relaxed);
    }
};

inline thread_local ThreadMetrics t_metrics;

}