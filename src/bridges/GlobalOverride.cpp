#include <new>
#include <cstdlib>
#include <cstdio>
#include <cstdint>
#include "../engine/FreeList.h"
#include "../core/MemoryBlock.h"

extern engine::FreeList* g_engine;
extern core::MemoryBlock* g_block;

constexpr std::size_t MAX_ALLOC_SIZE = 1024;

void* operator new(std::size_t size) {
    if (g_engine == nullptr || size > MAX_ALLOC_SIZE) {
        void* ptr = std::malloc(size);
        if (ptr == nullptr) {
            fprintf(stderr, "Fatal Error: No memory remaining.\n");
            std::abort();
        }
        return ptr;
    }

    void* ptr = g_engine->pop(size);
    if (ptr == nullptr) {
        fprintf(stderr, "Fatal Error: Engine memory depleted.\n");
        std::abort();
    }

    return ptr;
}

void operator delete(void* ptr) noexcept {
    if (ptr == nullptr) {
        return;
    }

    if (g_engine != nullptr && g_block != nullptr) {
        std::uintptr_t p = reinterpret_cast<std::uintptr_t>(ptr);
        
        if (p >= g_block->get_start_address() && p < g_block->get_end_address()) {
            g_engine->push(ptr);
            return;
        }
    }
    
    std::free(ptr);
}

void* operator new[](std::size_t size) {
    return ::operator new(size);
}

void operator delete[](void* ptr) noexcept {
    ::operator delete(ptr);
}

void operator delete(void* ptr, std::size_t /* size */) noexcept {
    ::operator delete(ptr);
}

void operator delete[](void* ptr, std::size_t /* size */) noexcept {
    ::operator delete(ptr);
}