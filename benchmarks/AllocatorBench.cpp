#include <benchmark/benchmark.h>
#include <vector>
#include <cstdlib>
#include <new>

// Benchmark 1: Our TLS Allocator
static void BM_CustomAllocator(benchmark::State& state) {
    for (auto _ : state) {
        void* ptr = ::operator new(state.range(0));
        benchmark::DoNotOptimize(ptr);
        ::operator delete(ptr);
    }
}
BENCHMARK(BM_CustomAllocator)->Range(32, 512)->Threads(8);

// Benchmark 2: glibc std::malloc
static void BM_SystemMalloc(benchmark::State& state) {
    for (auto _ : state) {
        void* ptr = std::malloc(state.range(0));
        benchmark::DoNotOptimize(ptr);
        std::free(ptr);
    }
}
BENCHMARK(BM_SystemMalloc)->Range(32, 512)->Threads(8);

// Benchmark 3: Burst Allocations
static void BM_CustomAllocator_Burst(benchmark::State& state) {
    std::vector<void*> ptrs(1000);
    for (auto _ : state) {
        for (int i = 0; i < 1000; ++i) {
            ptrs[i] = ::operator new(state.range(0));
        }
        benchmark::DoNotOptimize(ptrs);
        for (int i = 0; i < 1000; ++i) {
            ::operator delete(ptrs[i]);
        }
    }
}
BENCHMARK(BM_CustomAllocator_Burst)->Range(64, 256)->Threads(8);

static void BM_SystemMalloc_Burst(benchmark::State& state) {
    std::vector<void*> ptrs(1000);
    for (auto _ : state) {
        for (int i = 0; i < 1000; ++i) {
            ptrs[i] = std::malloc(state.range(0));
        }
        benchmark::DoNotOptimize(ptrs);
        for (int i = 0; i < 1000; ++i) {
            std::free(ptrs[i]);
        }
    }
}
BENCHMARK(BM_SystemMalloc_Burst)->Range(64, 256)->Threads(8);

BENCHMARK_MAIN();