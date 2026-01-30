#pragma once
#include <concepts>
#include <type_traits>
#include <iostream>

namespace MEM_SENTRY::concepts {
    /**
     * @brief Concept to prevent raw C-style arrays (e.g., int[]) from being used.
     * This ensures the pool manages single objects, std::array, or complex classes 
     * without the overhead of tracking array counts.
     */
    template<typename T>
    concept NotRawArray = !std::is_array_v<T>;
}
