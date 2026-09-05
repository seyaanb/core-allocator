#include "FreeList.h"
#include "../core/MemoryBlock.h"
#include "../telemetry/MemoryTracker.h"
#include <bit>
#include <algorithm>
#include <cassert>

namespace engine {

inline std::pair<size_t, size_t> get_size_class(size_t request_size) noexcept {
    size_t total_size = request_size + HEADER_SIZE;
    size_t chunk_size = std::max(MIN_CHUNK_SIZE, std::bit_ceil(total_size));
    size_t index = std::bit_width(chunk_size - 1) - 6;
    return {chunk_size, index};
}

void* FreeList::allocate(size_t size) noexcept {
    auto [chunk_size, index] = get_size_class(size);

    if (chunk_size > MAX_CHUNK_SIZE) return nullptr;

    if (m_heads[index] == nullptr) {
        void* new_block = fetch_slab_and_carve(chunk_size, index);
        if (new_block == nullptr) return nullptr; 
    }

    Node* node = m_heads[index];
    m_heads[index] = node->next;

    size_t* header = reinterpret_cast<size_t*>(node);
    *header = chunk_size;

    telemetry::t_metrics.allocs[index]++;

    return reinterpret_cast<std::byte*>(node) + HEADER_SIZE;
}

void FreeList::deallocate(void* ptr) noexcept {
    if (ptr == nullptr) return;

    std::byte* raw_ptr = static_cast<std::byte*>(ptr) - HEADER_SIZE;
    size_t chunk_size = *reinterpret_cast<size_t*>(raw_ptr);
    size_t index = std::bit_width(chunk_size - 1) - 6;

    Node* node = reinterpret_cast<Node*>(raw_ptr);
    node->next = m_heads[index];
    m_heads[index] = node;

    telemetry::t_metrics.deallocs[index]++;
}

void* FreeList::fetch_slab_and_carve(size_t chunk_size, size_t index) noexcept {
    void* slab = core::MemoryBlock::get_instance().allocate_slab(SLAB_SIZE);
    if (slab == nullptr) return nullptr;

    size_t num_chunks = SLAB_SIZE / chunk_size;
    std::byte* curr = static_cast<std::byte*>(slab);
    
    m_heads[index] = reinterpret_cast<Node*>(curr);
    Node* curr_node = m_heads[index];

    for (size_t i = 1; i < num_chunks; i++) {
        std::byte* next_addr = curr + chunk_size;
        Node* next_node = reinterpret_cast<Node*>(next_addr);
        curr_node->next = next_node;
        curr_node = next_node;
        curr = next_addr;
    }
    curr_node->next = nullptr;

    return m_heads[index]; 
}

}