#include <new>
#include <cstdlib>
#include <cstdio>
#include "../engine/FreeList.h"

extern engine::FreeList* g_engine;

constexpr std::size_t CHUNK_SIZE = 64;

void* operator new(std::size_t size) {
    if (g_engine == nullptr) {
        void* ptr = std::malloc(size);
        if (ptr == nullptr) {
            fprintf(stderr, "Fatal Error: No memory remaining.\n");
            std::abort();
        }
        return ptr;
    }

    if (size > CHUNK_SIZE) {
        fprintf(stderr, "Fatal Error: size greater than CHUNK_SIZE.\n");
        std::abort();
    }

    void* ptr = g_engine->pop();
    if (ptr == nullptr) {
        fprintf(stderr, "Fatal Error: No memory remaining.\n");
        std::abort();
    }

    return ptr;
}

void operator delete(void* ptr) noexcept {
    if (ptr == nullptr) {
        return;
    }

    //assumes ALL deletions after boot go to the engine

    if (g_engine != nullptr) {
        g_engine->push(ptr);
        return;
    }
    std::free(ptr);
    return;
}

void* operator new[](std::size_t size) {
    return ::operator new(size);
}

void operator delete[](void* ptr) noexcept {
    return ::operator delete(ptr);
}

void operator delete(void* ptr, std::size_t size) noexcept {
    return ::operator delete(ptr);
}

void operator delete[](void* ptr, std::size_t size) noexcept {
    return ::operator delete(ptr);
}
