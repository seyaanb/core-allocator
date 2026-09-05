#include "FreeList.h"
#include <cstddef>
#include <cassert>
#include <cstdint>
#include <bit>
#include <algorithm>
#include "../telemetry/MemoryTracker.h"

namespace engine {

FreeList::FreeList(void* base_ptr, size_t total_size) : m_base_ptr(base_ptr) {
    assert(reinterpret_cast<std::uintptr_t>(base_ptr) % 64 == 0 && "base_ptr is not aligned");

    m_region_size = (total_size / 5) & ~(size_t)1023;

    for (int i = 0; i < 5; ++i) {
        size_t chunk_size = 64 << i;
        size_t num_blocks = m_region_size / chunk_size;
        
        char* region_base = static_cast<char*>(m_base_ptr) + (i * m_region_size);
        m_heads[i] = reinterpret_cast<Node*>(region_base);
        
        Node* curr = m_heads[i];
        for (size_t j = 1; j < num_blocks; j++) {
            Node* next_node = reinterpret_cast<Node*>(region_base + (j * chunk_size));
            curr->next = next_node;
            curr = next_node;
        }
        curr->next = nullptr;
    }
}

void* FreeList::pop(size_t size) noexcept {
    if (size > 1024) {
        return nullptr;
    }

    size_t rounded_size = std::max<size_t>(64, std::bit_ceil(size));
    
    int index = std::countr_zero(rounded_size) - 6;

    if (m_heads[index] == nullptr) {
        return nullptr;
    }

    void* alloc = static_cast<void*>(m_heads[index]);
    m_heads[index] = m_heads[index]->next;
    
    telemetry::MemoryTracker::record_allocation();
    return alloc;
}

void FreeList::push(void* ptr) noexcept {
    if (ptr == nullptr) {
        return;
    }

    std::uintptr_t p = reinterpret_cast<std::uintptr_t>(ptr);
    std::uintptr_t base = reinterpret_cast<std::uintptr_t>(m_base_ptr);
    
    size_t index = (p - base) / m_region_size;
    if (index >= 5) {
        return;
    }

    Node* node = static_cast<Node*>(ptr);
    node->next = m_heads[index];
    m_heads[index] = node;
    
    telemetry::MemoryTracker::record_deallocation();
}

}