#include <iostream>
#include <string>
#include "core/MemoryBlock.h"
#include "engine/FreeList.h"
#include "raii/SmartPtr.h"
#include "telemetry/MemoryTracker.h"
#include "telemetry/Alignment.h"

engine::FreeList* g_engine = nullptr;
core::MemoryBlock* g_block = nullptr;

// Test Structs
struct SmallStruct { char data[12]; };   // Should hit 64-byte list
struct MediumStruct { char data[100]; }; // Should hit 128-byte list
struct LargeStruct { char data[500]; };  // Should hit 512-byte list
struct MassiveStruct { char data[2000]; }; // Should bypass to std::malloc

int main() {
    constexpr size_t ONE_GIGABYTE = 1024 * 1024 * 1024;

    std::cout << "Booting HFT Allocator...\n";
    
    core::MemoryBlock block(ONE_GIGABYTE);
    engine::FreeList engine(block.get_base_pointer(), block.get_size());
    
    g_engine = &engine;
    g_block = &block;
    std::cout << "Engine activated with Size Class Segregation.\n\n";

    std::cout << "--- Iteration 2: Size Class Validation ---\n";
    
    SmallStruct* s1 = new SmallStruct();
    MediumStruct* m1 = new MediumStruct();
    LargeStruct* l1 = new LargeStruct();
    MassiveStruct* mass1 = new MassiveStruct(); // Routes to std::malloc
    
    std::cout << "Objects allocated successfully.\n";

    auto check_ownership = [&](void* ptr, const char* name) {
        std::uintptr_t p = reinterpret_cast<std::uintptr_t>(ptr);
        if (p >= g_block->get_start_address() && p < g_block->get_end_address()) {
            std::cout << name << " is owned by FreeList.\n";
        } else {
            std::cout << name << " is owned by std::malloc.\n";
        }
    };

    check_ownership(s1, "SmallStruct (12b)");
    check_ownership(m1, "MediumStruct (100b)");
    check_ownership(l1, "LargeStruct (500b)");
    check_ownership(mass1, "MassiveStruct (2000b)"); // Expected: malloc

    delete s1;
    delete m1;
    delete l1;
    delete mass1;
    
    std::cout << "\n--- Telemetry Report ---\n";
    std::cout << "Pool Allocations: " << telemetry::MemoryTracker::thread_allocations << "\n";
    std::cout << "Pool Deallocations: " << telemetry::MemoryTracker::thread_deallocations << "\n";

    g_engine = nullptr;
    g_block = nullptr;
    std::cout << "\nShutting down.\n";
    
    return 0;
}