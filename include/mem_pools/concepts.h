#pragma once
#include <concepts>
#include <type_traits>
#include <iostream>

namespace MEM_SENTRY::concepts {
    template<typename T>
    concept NotRawArray = !std::is_array_v<T>;
}
