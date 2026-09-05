#pragma once

#include <cstddef>
#include <cstdint>
#include <atomic>

namespace core {

class MemoryBlock {
    public:
        static MemoryBlock& get_instance() noexcept;

        // Deleted copy/move semantics
        MemoryBlock(const MemoryBlock&) = delete;
        MemoryBlock& operator=(const MemoryBlock&) = delete;
        MemoryBlock(MemoryBlock&&) = delete;
        MemoryBlock& operator=(MemoryBlock&&) = delete;

        void* allocate_slab(size_t slab_size) noexcept;

        std::uintptr_t get_start_address() const noexcept;
        std::uintptr_t get_end_address() const noexcept;
        
    private:
        explicit MemoryBlock(size_t total_bytes);
        ~MemoryBlock();

        void* m_base_ptr;
        size_t m_size;
        
        alignas(64) std::atomic<size_t> m_global_offset;
};

}