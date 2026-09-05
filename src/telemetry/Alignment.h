#pragma once
#include <cstddef>
#include <cstdint>

namespace telemetry {
    constexpr std::size_t CACHE_LINE_SIZE = 64;

    inline std::size_t align_size(std::size_t size) noexcept {
        return (size + CACHE_LINE_SIZE - 1) & ~(CACHE_LINE_SIZE - 1);
    }

    inline bool is_aligned(const void* ptr) noexcept {
        return (reinterpret_cast<std::uintptr_t>(ptr) & (CACHE_LINE_SIZE - 1)) == 0;
    }
}