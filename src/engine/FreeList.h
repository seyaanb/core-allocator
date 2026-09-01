#pragma once

#include <cstddef>
#include <cstdint>

namespace engine {

class FreeList {
    public:
        FreeList(void* base_ptr, size_t total_size, size_t chunk_size);

        FreeList(const FreeList&) = delete;
        FreeList& operator=(const FreeList&) = delete;
        FreeList(FreeList&&) = delete;
        FreeList& operator=(FreeList&&) = delete;

        void* pop() noexcept;
        void push(void* ptr) noexcept;
    
    private:
        struct Node {
            Node* next;
        };

        Node* m_head;
};

}