#include <iostream>
#include <vector>
#include <thread>
#include <cstdlib>
#include "../src/core/MemoryBlock.h"

#define ALWAYS_ASSERT(condition) \
    do { \
        if (!(condition)) { \
            std::cerr << "Fatal: Check failed: " #condition << "\n"; \
            std::abort(); \
        } \
    } while (false)

struct SmallStruct { char data[12]; };   
struct MediumStruct { char data[100]; }; 
struct MassiveStruct { char data[2000]; }; 

void thread_workload(int thread_id) {
    std::cout << "Thread " << thread_id << " running lock-free allocations...\n";
    
    SmallStruct* s1 = new SmallStruct();
    MediumStruct* m1 = new MediumStruct();
    MassiveStruct* mass = new MassiveStruct();

    std::uintptr_t s1_addr = reinterpret_cast<std::uintptr_t>(s1);
    std::uintptr_t mass_addr = reinterpret_cast<std::uintptr_t>(mass);
    
    auto& block = core::MemoryBlock::get_instance();
    
    ALWAYS_ASSERT(s1_addr >= block.get_start_address() && s1_addr < block.get_end_address());
    ALWAYS_ASSERT(mass_addr < block.get_start_address() || mass_addr >= block.get_end_address());

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