#include "MemoryBlock.h"
#include <sys/mman.h>
#include <cstdlib>
#include <cstdio>

namespace core {

MemoryBlock::MemoryBlock(size_t total_bytes) 
    : m_base_ptr{ nullptr }, m_size{ total_bytes } {
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

void* MemoryBlock::get_base_pointer() const noexcept {
    return m_base_ptr;
}

size_t MemoryBlock::get_size() const noexcept {
    return m_size;
}

std::uintptr_t MemoryBlock::get_start_address() const noexcept {
    return reinterpret_cast<std::uintptr_t>(m_base_ptr);
}

std::uintptr_t MemoryBlock::get_end_address() const noexcept {
    return reinterpret_cast<std::uintptr_t>(m_base_ptr) + m_size;
}

}