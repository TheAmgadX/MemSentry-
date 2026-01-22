#pragma once
#include <iostream>
#include <cstring>
#include <atomic>
#include <mutex>

#include "mem_sentry/alloc_header.h"
#include "mem_sentry/reporter.h"

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
        std::atomic<int> m_NextAllocId;

        /** @brief Pointer to the first allocation in the tracking list. */
        alloc_header::AllocHeader* p_HeadList;

        /** @brief Pointer to the last allocation in the tracking list. */
        alloc_header::AllocHeader* p_TailList;

        reporter::IReporter* p_Reporter;
        
        /**
         * @brief linked list mutex.
         * 
         * used to make sure the heap list is thread safe.
         */
        std::mutex m_llMutex;
        
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
            p_Reporter = nullptr;
        }
        
        void SetReporter(reporter::IReporter* reporter){
            p_Reporter = reporter;
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
        int GetNextId() { 
            return m_NextAllocId.fetch_add(1, std::memory_order_relaxed); 
        }

        /**
         * @brief Returns the current total bytes allocated on this heap.
         */
        int GetTotal() const noexcept { return m_total; }

        /**
         * @brief Count active allocations tracked by this heap.
         */
        int CountAllocations() noexcept;

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