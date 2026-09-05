#pragma once

#include <cstddef>
#include <cstdint>
#include <array>

namespace engine {

constexpr size_t HEADER_SIZE = 16;
constexpr size_t MIN_CHUNK_SIZE = 64;
constexpr size_t MAX_CHUNK_SIZE = 1024;
constexpr size_t SLAB_SIZE = 64 * 1024;

class FreeList {
    public:
        FreeList() = default;
        ~FreeList() = default;

        FreeList(const FreeList&) = delete;
        FreeList& operator=(const FreeList&) = delete;
        FreeList(FreeList&&) = delete;
        FreeList& operator=(FreeList&&) = delete;

        void* allocate(size_t size) noexcept;
        void deallocate(void* ptr) noexcept;
    
    private:
        struct Node {
            Node* next;
        };

        std::array<Node*, 5> m_heads{nullptr, nullptr, nullptr, nullptr, nullptr};

        void* fetch_slab_and_carve(size_t chunk_size, size_t index) noexcept;
};

}