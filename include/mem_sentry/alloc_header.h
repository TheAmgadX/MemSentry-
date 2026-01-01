#pragma once 
#include "mem_sentry/heap.h"

namespace MEM_SENTRY::alloc_header {
    struct AllocHeader {
        /// @brief pointer to the heap that tracks this allocation.
        MEM_SENTRY::heap::Heap* pHeap;
        
        /// @brief size of the allocated memory without header and end marker.
        int iSize;

        /// @brief to make sure we don't free data that is not allocated by our memory manager.
        int iSignature;

        /// @brief pointer to the next allocation header.
        AllocHeader* pNext;

        /// @brief pointer to the previous allocation header.
        AllocHeader* pPrev;
    };
};
