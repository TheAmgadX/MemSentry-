#pragma once 
#include "mem_sentry/heap.h"

namespace MEM_SENTRY::alloc_header {
    struct AllocHeader {
        /// @brief pointer to the heap that tracks this allocation.
        MEM_SENTRY::heap::Heap* p_Heap;
        
        /// @brief size of the allocated memory without header and end marker.
        int m_Size;

        /// @brief to make sure we don't free data that is not allocated by our memory manager.
        int m_Signature;

        /// @brief pointer to the next allocation header.
        AllocHeader* p_Next;

        /// @brief pointer to the previous allocation header.
        AllocHeader* p_Prev;

        /// @brief allocation number to track the allocations bookmarks.
        int m_AllocId;
    };
};
