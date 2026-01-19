#pragma once
#include <iostream>
#include <cstring>
#include "mem_sentry/alloc_header.h"

namespace MEM_SENTRY::heap {       
    
    /**
     * @class Heap
     * @brief Manages a specific memory arena (category).
     *
     * The Heap class tracks memory statistics (total bytes) and maintains 
     * a doubly-linked list of all active allocations belonging to this category.
     * It allows for detailed reporting and leak detection.
     * 
     */
    class Heap {
    private:
        /** @brief Name of the heap (e.g., "Physics", "AI"). */
        char m_name[100];

        /** @brief Total bytes currently allocated in this heap. */
        int m_total;
        
        /** @brief Counter to generate unique IDs for allocations. */
        int m_NextAllocId;

        /** @brief Pointer to the first allocation in the tracking list. */
        alloc_header::AllocHeader* p_HeadList;

        /** @brief Pointer to the last allocation in the tracking list. */
        alloc_header::AllocHeader* p_TailList;

        /**
         * @brief Internal helper to print details of a specific allocation.
         * @param alloc Pointer to the allocation header to print.
         */
        void printAlloc(alloc_header::AllocHeader* alloc, int totalMemoryToThisPoint);
        
        /**
         * @brief Internal helper to append a node to the linked list.
         * @param alloc Pointer to the new header to add.
         * @return true if successful, false otherwise.
         */ 
        bool addAllocLL(alloc_header::AllocHeader* alloc);

        /**
         * @brief Internal helper to unlink a node from the linked list.
         * @note This does NOT free the memory, it only removes it from tracking, removal happens in overridden delete.
         * @param alloc Pointer to the header to remove.
         * @return true if found and removed, false otherwise.
         */
        bool removeAllocLL(alloc_header::AllocHeader* alloc);

    public:
        /**
         * @brief Construct a new Heap object.
         * @param name The display name for this memory category.
         */
        Heap(const char *name) {
            std::strncpy(m_name, name, 99);
            m_name[99] = '\0';
            m_total = 0;
            m_NextAllocId = 1;

            p_HeadList = nullptr;
            p_TailList = nullptr;
        }
        
        /**
         * @brief Get the name of this heap.
         * @return const char* The name string.
         */
        const char * GetName() const noexcept { return m_name; }
        
        /**
         * @brief return a unique Id for a new allocation and increments the counter.
         * @return int The new Allocation Id.
         */
        int GetNextId() { return m_NextAllocId++; }

        /**
         * @brief Returns the current total bytes allocated on this heap.
         */
        int GetTotal() const noexcept { return m_total; }

        /**
         * @brief Count active allocations tracked by this heap.
         */
        int CountAllocations() const noexcept;

        /**
         * @brief Registers a new allocation with this heap.
         * * Updates the total byte count and adds the header to the internal linked list.
         * 
         * @param alloc Pointer to the header of the newly allocated memory.
         */
        void AddAllocation(alloc_header::AllocHeader* alloc);
    
        /**
         * @brief Unregisters an allocation from this heap.
         * * Decreases total byte count and removes the header from the internal linked list.
         * 
         * @param alloc Pointer to the header of the memory being freed.
         */
        void RemoveAlloc(alloc_header::AllocHeader* alloc);

        /**
         * @brief Prints all active allocations between two IDs.
         * * Used to detect leaks or inspect memory usage between two points in time.
         * 
         * @param bookMark1 The starting Allocation ID (inclusive).
         * @param bookMark2 The ending Allocation ID (inclusive).
         */
        void ReportMemory(int bookMark1, int bookMark2);
    };
    
    /**
     * @class HeapFactory
     * @brief Static provider for the system default heap.
     */
    class HeapFactory {
    public:
        /**
         * @brief Retrieves the singleton Default Heap.
         * @return Heap* Pointer to the global default heap.
         */
        static Heap* GetDefaultHeap() {
            static Heap defaultHeap("DefaultHeap");
            return &defaultHeap;
        }
    };
};