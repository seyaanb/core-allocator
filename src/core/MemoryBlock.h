#pragma once

#include <cstddef>
#include <cstdint>

namespace core {

class MemoryBlock {
    public:
        explicit MemoryBlock(size_t total_bytes);

        ~MemoryBlock();

        //Copy constructor
        MemoryBlock(const MemoryBlock&) = delete;
        //Copy assignment
        MemoryBlock& operator=(const MemoryBlock&) = delete;
        //Move constructor
        MemoryBlock(MemoryBlock&&) = delete;
        //Move assignment
        MemoryBlock& operator=(MemoryBlock&&) = delete;

        void* get_base_pointer() const noexcept;
        size_t get_size() const noexcept;

        std::uintptr_t get_start_address() const noexcept;
        std::uintptr_t get_end_address() const noexcept;
    
    private:
        void* m_base_ptr;
        size_t m_size;
};

}