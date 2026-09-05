#pragma once

#include <memory>
#include <cstdlib>
#include <cstdio>
#include "../engine/FreeList.h"

extern engine::FreeList* g_engine;

namespace raii {

template <typename T>
struct EngineDeleter {
    void operator()(T* ptr) const noexcept {
        if (ptr == nullptr) {
            return;
        }
        ptr->~T();
        if (g_engine) {
            g_engine->push(static_cast<void*>(ptr));
        } else {
            std::free(static_cast<void*>(ptr));
        }
    }
};

template <typename T>
using CorePtr = std::unique_ptr<T, EngineDeleter<T>>;

template <typename T, typename... Args>
CorePtr<T> make_core(Args&&... args) {
    static_assert(sizeof(T) <= 1024, "Fatal: Object size exceeds max pool size");

    void* raw_memory_ptr{};
    if (g_engine) {
        raw_memory_ptr = g_engine->pop(sizeof(T));
    } else {
        raw_memory_ptr = std::malloc(sizeof(T));
    }
    if (raw_memory_ptr == nullptr) {
        fprintf(stderr, "Fatal Error: Insufficient memory.\n");
        std::abort();
    }
    T* object_ptr = new (raw_memory_ptr) T(std::forward<Args>(args)...);
    CorePtr<T> ptr { object_ptr };
    return ptr; 
}


}