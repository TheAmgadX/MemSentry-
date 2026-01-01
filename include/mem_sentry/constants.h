#pragma once

namespace MEM_SENTRY::constants {
    /// @brief signature value to check if the data we gonna free is allocated by our memory manager.
    constexpr int MEMSYSTEM_SIGNATURE = 0xDEADC0DE;

    /// @brief endmarker to make sure we don't free beyond the array.
    constexpr int MEMSYSTEM_ENDMARKER = 0XEEDC0DE;
};