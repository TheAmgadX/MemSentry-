#pragma once 
#include <cstdint>

namespace MEM_SENTRY::heap {
    class Heap;
}

namespace MEM_SENTRY::alloc_header {
    /**
     * @struct AllocHeader
     * @brief Metadata header attached to every allocation.
     *
     * This header acts as a node in the memory tracking doubly-linked list. 
     * It stores ownership details, integrity signatures, and the original pointer
     * required to correctly free aligned memory.
     *
     * @note Memory Layout:
     * - Pointers (32 bytes): p_Heap, p_Next, p_Prev, p_OriginalAddress
     * - Integers (13 bytes): m_Size(4), m_Signature(4), m_AllocId(4), m_Alignment(1)
     * - Padding  (3 bytes):  To align struct to 8-byte boundary.
     * - Total Size: 48 Bytes.
     */
    struct AllocHeader {
        // --- Pointers (8 bytes each) ---

        /// @brief Pointer to the heap that tracks this allocation.
        MEM_SENTRY::heap::Heap* p_Heap;

        /// @brief Pointer to the next allocation in the linked list.
        AllocHeader* p_Next;

        /// @brief Pointer to the previous allocation in the linked list.
        AllocHeader* p_Prev;

        /// @brief Original raw pointer returned by malloc.
        /// Essential for freeing aligned allocations where the user pointer is offset.
        void* p_OriginalAddress;

        // --- Data Fields ---

        /// @brief Size of the user data (excluding header/footer).
        uint32_t m_Size;

        /// @brief Integrity signature (Active vs Freed).
        /// Used to detect corruption or double-free errors.
        uint32_t m_Signature;

        /// @brief Unique allocation ID for tracking/reporting.
        uint32_t m_AllocId;

        /// @brief Alignment used for this allocation.
        uint8_t m_Alignment;

        // 3 bytes of implicit padding here on 64-bit systems
    };
};