#pragma once

#include <memory>
#include <utility>

namespace raii {

template <typename T>
using CorePtr = std::unique_ptr<T>;

template <typename T, typename... Args>
CorePtr<T> make_core(Args&&... args) {
    static_assert(sizeof(T) <= 1008, "Fatal: Object size exceeds maximum TLS chunk size");
    
    return std::make_unique<T>(std::forward<Args>(args)...);
}

}