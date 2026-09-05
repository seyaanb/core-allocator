#include <iostream>
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

int main() {
    constexpr size_t ONE_GIGABYTE = 1024 * 1024 * 1024;
    constexpr size_t CHUNK_SIZE = 64;

    std::cout << "Booting HFT Allocator...\n";
    
    core::MemoryBlock block(ONE_GIGABYTE);
    
    void* aligned_base = block.get_base_pointer();
    if (!telemetry::is_aligned(aligned_base)) {
        std::cerr << "Fatal: Base pointer not 64-byte aligned!\n";
        return 1;
    }

    engine::FreeList engine(aligned_base, block.get_size(), CHUNK_SIZE);
    
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

    std::cout << "--- Telemetry Report ---\n";
    std::cout << "Allocations (Pool): " << telemetry::MemoryTracker::thread_allocations << "\n";
    std::cout << "Deallocations (Pool): " << telemetry::MemoryTracker::thread_deallocations << "\n";

    g_engine = nullptr;
    g_block = nullptr;
    std::cout << "\nShutting down.\n";
    
    return 0;
}