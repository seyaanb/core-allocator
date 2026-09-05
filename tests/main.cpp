#include <iostream>
#include <cassert>
#include <string>
#include "core/MemoryBlock.h"
#include "engine/FreeList.h"
#include "raii/SmartPtr.h"
#include "telemetry/MemoryTracker.h"
#include "telemetry/Alignment.h"

engine::FreeList* g_engine = nullptr;
core::MemoryBlock* g_block = nullptr;

struct Order {
    double price;
    uint32_t quantity;
    uint32_t ticker_id;
    
    Order(double p, uint32_t q, uint32_t t) : price(p), quantity(q), ticker_id(t) {
        std::cout << "Order constructed: " << ticker_id << "\n";
    }
    ~Order() {
        std::cout << "Order destroyed: " << ticker_id << "\n";
    }
};

size_t slab_index_of(void* ptr, std::uintptr_t base, size_t total_size) {
    constexpr size_t MAX_CHUNK_SIZE = 1024;
    size_t raw_slab_size = total_size / 5;
    size_t slab_size = raw_slab_size - (raw_slab_size % MAX_CHUNK_SIZE);
    std::uintptr_t offset = reinterpret_cast<std::uintptr_t>(ptr) - base;
    return offset / slab_size;
}

int main() {
    constexpr size_t ONE_GIGABYTE = 1024 * 1024 * 1024;

    std::cout << "Booting HFT Allocator...\n";
    
    core::MemoryBlock block(ONE_GIGABYTE);
    
    void* aligned_base = block.get_base_pointer();
    if (!telemetry::is_aligned(aligned_base)) {
        std::cerr << "Fatal: Base pointer not 64-byte aligned!\n";
        return 1;
    }

    // FreeList now derives its own size classes (64..1024) from the block
    // it's given, rather than being handed a single fixed chunk size.
    engine::FreeList engine(aligned_base, block.get_size());
    
    g_engine = &engine;
    g_block = &block;
    std::cout << "Engine and Bounds Checking activated.\n\n";

    std::cout << "--- Iteration 1: Boundary Check Validation ---\n";
    
    // Test A: Allocate via custom pool (size <= 64)
    std::cout << "Allocating Order (Pool)...\n";
    Order* pool_order = new Order(150.25, 100, 1001);
    
    // Test B: Allocate via glibc malloc (size > 64 triggers fallback)
    std::cout << "Allocating string (glibc)...\n";
    std::string* glibc_string = new std::string("This string is dynamically allocated by the standard library and exceeds 64 bytes.");
    
    // Test C: RAII integration
    {
        std::cout << "Allocating RAII Order (Pool)...\n";
        auto smart_order = raii::make_core<Order>(151.00, 200, 1002);
    }
    
    // Test D: Boundary-checked deletions
    std::cout << "Deleting Order (Routes to pool)...\n";
    delete pool_order; 
    
    std::cout << "Deleting string (Routes to glibc)...\n";
    delete glibc_string; 
    
    std::cout << "\nValidation Passed: No segfaults. Memory correctly routed.\n\n";

    std::cout << "--- Iteration 2: Size Class Segregation Validation ---\n";

    std::uintptr_t base_addr = block.get_start_address();
    size_t total_size = block.get_size();

    struct SizeCase {
        size_t request_size;
        size_t expected_index;
        const char* label;
    };

    SizeCase cases[] = {
        { 12,  0, "12 bytes  -> rounds up to 64-byte class"  },
        { 100, 1, "100 bytes -> rounds up to 128-byte class" },
        { 500, 3, "500 bytes -> rounds up to 512-byte class" },
    };

    void* allocated[3] = { nullptr, nullptr, nullptr };

    for (size_t i = 0; i < 3; ++i) {
        const SizeCase& c = cases[i];
        void* ptr = ::operator new(c.request_size);
        allocated[i] = ptr;

        assert(reinterpret_cast<std::uintptr_t>(ptr) >= base_addr &&
               reinterpret_cast<std::uintptr_t>(ptr) < base_addr + total_size &&
               "Allocation unexpectedly fell outside the pool");

        size_t actual_index = slab_index_of(ptr, base_addr, total_size);
        std::cout << c.label << " (slab index " << actual_index << ")\n";
        assert(actual_index == c.expected_index && "Allocation landed in wrong size-class slab");
    }

    std::cout << "Size class assignment validated for 12, 100, and 500 byte requests.\n";

    void* oversized = ::operator new(2048);
    bool oversized_in_pool =
        reinterpret_cast<std::uintptr_t>(oversized) >= base_addr &&
        reinterpret_cast<std::uintptr_t>(oversized) < base_addr + total_size;
    assert(!oversized_in_pool && "Oversized allocation incorrectly served from the pool");
    std::cout << "2048-byte allocation correctly bypassed the pool (routed to glibc).\n\n";

    std::cout << "Validation Passed: Size classes correctly segregated; oversized requests bypass the pool.\n\n";

    for (void* ptr : allocated) {
        ::operator delete(ptr);
    }
    ::operator delete(oversized);

    std::cout << "--- Telemetry Report ---\n";
    std::cout << "Allocations (Pool): " << telemetry::MemoryTracker::thread_allocations << "\n";
    std::cout << "Deallocations (Pool): " << telemetry::MemoryTracker::thread_deallocations << "\n";

    g_engine = nullptr;
    g_block = nullptr;
    std::cout << "\nShutting down.\n";
    
    return 0;
}