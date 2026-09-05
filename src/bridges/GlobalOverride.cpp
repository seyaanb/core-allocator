#include <new>
#include <cstdlib>
#include <cstdio>
#include "../engine/FreeList.h"
#include "../core/MemoryBlock.h"
#include "../telemetry/MemoryTracker.h"

thread_local engine::FreeList tls_engine;

struct TelemetryBootstrapper {
    TelemetryBootstrapper() {
        std::atexit(telemetry::GlobalMetrics::print_report);
    }
};
static TelemetryBootstrapper g_telemetry_boot;

void* operator new(std::size_t size) {
    void* ptr = tls_engine.allocate(size);
    if (ptr != nullptr) return ptr;

    telemetry::t_metrics.fallback_mallocs++;
    ptr = std::malloc(size);
    if (ptr == nullptr) {
        fprintf(stderr, "Fatal Error: No memory remaining.\n");
        std::abort();
    }
    return ptr;
}

void operator delete(void* ptr) noexcept {
    if (ptr == nullptr) return;

    std::uintptr_t addr = reinterpret_cast<std::uintptr_t>(ptr);
    auto& mem_block = core::MemoryBlock::get_instance();

    if (addr >= mem_block.get_start_address() && addr < mem_block.get_end_address()) {
        tls_engine.deallocate(ptr);
    } else {
        std::free(ptr);
    }
}

void* operator new[](std::size_t size) { return ::operator new(size); }
void operator delete[](void* ptr) noexcept { ::operator delete(ptr); }
void operator delete(void* ptr, std::size_t) noexcept { ::operator delete(ptr); }
void operator delete[](void* ptr, std::size_t) noexcept { ::operator delete(ptr); }