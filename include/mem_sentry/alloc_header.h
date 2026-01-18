#pragma once 

namespace MEM_SENTRY::heap {
    class Heap;
}

namespace MEM_SENTRY::alloc_header {
    /**
     * @struct AllocHeader
     * @brief Metadata header attached to every allocation.
     *
     * This header acts as a node in the memory tracking linked list. It allows
     * the system to identify which heap owns the memory, validate memory integrity
     * via signatures, and traverse active allocations.
     *
     * @note: 
     * - Size: 40 Bytes (on 64-bit architecture).
     * - Layout: Contains 3 pointers (24B) and 3 integers (12B) + 4 bytes padding.
     */
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
