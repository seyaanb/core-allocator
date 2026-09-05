#include "FreeList.h"
#include <cassert>
#include <algorithm>
#include "../telemetry/MemoryTracker.h"

namespace engine {

namespace {
    
    constexpr size_t MAX_CHUNK_SIZE = 1024;

    size_t compute_aligned_slab_size(size_t total_size) {
        size_t raw_slab_size = total_size / 5;
        return raw_slab_size - (raw_slab_size % MAX_CHUNK_SIZE);
    }
}

FreeList::FreeList(void* base_ptr, size_t total_size) 
    : m_heads{nullptr, nullptr, nullptr, nullptr, nullptr}, 
      m_base_ptr{base_ptr}, 
      m_slab_size{compute_aligned_slab_size(total_size)} {
    
    assert(reinterpret_cast<std::uintptr_t>(base_ptr) % 64 == 0 && "base_ptr is not aligned");
    assert(m_slab_size % MAX_CHUNK_SIZE == 0 && "slab_size must be a multiple of the largest chunk size");

    for (size_t i = 0; i < 5; ++i) {
        size_t chunk_size = 64 << i;
        size_t num_blocks = m_slab_size / chunk_size;
        
        char* slab_start = static_cast<char*>(base_ptr) + (i * m_slab_size);
        m_heads[i] = reinterpret_cast<Node*>(slab_start);
        
        Node* curr = m_heads[i];
        for (size_t j = 1; j < num_blocks; j++) {
            Node* next_node = reinterpret_cast<Node*>(slab_start + (j * chunk_size));
            curr->next = next_node;
            curr = next_node;
        }
        curr->next = nullptr;
    }
}

size_t FreeList::get_index(size_t size) const noexcept {
    size_t rounded = std::bit_ceil(size);
    if (rounded < 64) rounded = 64;
    
    return std::countr_zero(rounded) - 6;
}

void* FreeList::pop(size_t size) noexcept {
    if (size > 1024) return nullptr;

    size_t index = get_index(size);
    if (m_heads[index] == nullptr) return nullptr;

    void* alloc = static_cast<void*>(m_heads[index]);
    m_heads[index] = m_heads[index]->next;
    
    telemetry::MemoryTracker::record_allocation();
    return alloc;
}

void FreeList::push(void* ptr) noexcept {
    if (ptr == nullptr) return;

    std::uintptr_t offset = reinterpret_cast<std::uintptr_t>(ptr) - reinterpret_cast<std::uintptr_t>(m_base_ptr);
    size_t index = offset / m_slab_size;
    assert(index < 5 && "push() received a pointer outside this FreeList's slabs");
    if (index >= 5) return; // defense in depth if asserts are compiled out

    Node* node = static_cast<Node*>(ptr);
    node->next = m_heads[index];
    m_heads[index] = node;
    
    telemetry::MemoryTracker::record_deallocation();
}

}