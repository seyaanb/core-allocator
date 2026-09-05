#include "MemoryBlock.h"
#include <sys/mman.h>
#include <cstdlib>
#include <cstdio>

namespace core {

MemoryBlock& MemoryBlock::get_instance() noexcept {
    static MemoryBlock instance(1024 * 1024 * 1024);
    return instance;
}

MemoryBlock::MemoryBlock(size_t total_bytes) 
    : m_base_ptr{ nullptr }, m_size{ total_bytes }, m_global_offset{ 0 } {
        m_base_ptr = mmap(nullptr, m_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_POPULATE, -1, 0);
        if (m_base_ptr == MAP_FAILED) {
            fprintf(stderr, "Fatal: Memory map failed. Aborting.\n");
            std::abort();
        }
}

MemoryBlock::~MemoryBlock() {
    if (m_base_ptr != nullptr && m_base_ptr != MAP_FAILED) {
        (void)munmap(m_base_ptr, m_size);
    }
}

void* MemoryBlock::allocate_slab(size_t slab_size) noexcept {
    size_t current_offset = m_global_offset.fetch_add(slab_size, std::memory_order_relaxed);
    
    if (current_offset + slab_size > m_size) {
        return nullptr; 
    }
    
    return static_cast<std::byte*>(m_base_ptr) + current_offset;
}

std::uintptr_t MemoryBlock::get_start_address() const noexcept {
    return reinterpret_cast<std::uintptr_t>(m_base_ptr);
}

std::uintptr_t MemoryBlock::get_end_address() const noexcept {
    return reinterpret_cast<std::uintptr_t>(m_base_ptr) + m_size;
}

}