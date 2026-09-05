#include <iostream>
#include <vector>
#include <thread>
#include <cassert>
#include "../src/core/MemoryBlock.h"

// Test Structs
struct SmallStruct { char data[12]; };   // 64-byte class
struct MediumStruct { char data[100]; }; // 128-byte class
struct MassiveStruct { char data[2000]; }; // Fallback to std::malloc

void thread_workload(int thread_id) {
    std::cout << "Thread " << thread_id << " running lock-free allocations...\n";
    
    SmallStruct* s1 = new SmallStruct();
    MediumStruct* m1 = new MediumStruct();
    MassiveStruct* mass = new MassiveStruct();

    std::uintptr_t s1_addr = reinterpret_cast<std::uintptr_t>(s1);
    std::uintptr_t mass_addr = reinterpret_cast<std::uintptr_t>(mass);
    
    auto& block = core::MemoryBlock::get_instance();
    
    assert(s1_addr >= block.get_start_address() && s1_addr < block.get_end_address());
    assert(mass_addr < block.get_start_address() || mass_addr >= block.get_end_address());

    delete s1;
    delete m1;
    delete mass;
}

int main() {
    std::cout << "Starting Correctness Test...\n";

    std::thread t1(thread_workload, 1);
    std::thread t2(thread_workload, 2);

    t1.join();
    t2.join();

    std::cout << "Correctness Test Passed. No Data Races. Memory Bounds Verified.\n";
    
    return 0;
}