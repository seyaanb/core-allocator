#include "FreeList.h"
#include <cstddef>
#include <cassert>
#include <cstdint>

namespace engine {

FreeList::FreeList(void* base_ptr, size_t total_size, size_t chunk_size) : m_head{ nullptr } {
    assert(reinterpret_cast<std::uintptr_t>(base_ptr) % alignof(Node) == 0 && "base_ptr is not aligned");

    if (total_size < chunk_size || chunk_size < sizeof(Node)) {
        m_head = nullptr;
        return;
    }

    size_t num_blocks = total_size / chunk_size;
    m_head = static_cast<Node*>(base_ptr);
    Node* curr = m_head;

    for (size_t i = 1; i < num_blocks; i++) {
        Node* next_node = reinterpret_cast<Node*>(reinterpret_cast<std::byte*>(curr) + chunk_size);
        curr->next = next_node;
        curr = next_node;
    }
    curr->next = nullptr;
}

void* FreeList::pop() noexcept {
    if (m_head == nullptr) {
        return nullptr;
    }

    void* alloc = static_cast<void*>(m_head);
    m_head = m_head->next;
    return alloc;
}

void FreeList::push(void* ptr) noexcept {
    if (ptr == nullptr) {
        return;
    }
    Node* node = static_cast<Node*>(ptr);
    node->next = m_head;
    m_head = node;
}

}