#pragma once
#include <iostream>
#include <cstring>
#include <atomic>
#include <mutex>
#include <vector>
#include <functional>
#include <unordered_set>

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

        /**
         * @brief Pointer to the reporter interface for logging memory events.
         * @note Can be nullptr if reporting is disabled.
         */
        reporter::IReporter* p_Reporter;

        /**
         * @brief Adjacency list storing pointers to connected neighbor heaps.
         * @note Used for graph traversal operations like GetTotalHH().
         */
        std::vector<Heap*> m_AdjHeaps;

        /**
         * @brief linked list mutex.
         * 
         * used to make sure the heap list is thread safe.
         */
        std::mutex m_llMutex;
        
        /**
         * @brief GLOBAL lock for the Heap Hierarchy.
         * Locks ALL heaps to prevent neighbor race conditions.
         */
        static std::mutex m_graphMutex;

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

        /**
         * @brief Helper function to perform Depth First Search (DFS) on the heap graph.
         * Traverses connected heaps recursively, maintaining a visited set to handle 
         * cycles in the graph (which occur naturally due to bidirectional connections).
         * 
         * @param heap The current heap node being visited.
         * @param visited Set of already visited heaps to prevent infinite loops.
         * @param val Reference to the accumulator value (size or count).
         * @param func The operation to perform on each visited heap (e.g., add size to val).
         */
        void dfs(Heap* heap, std::unordered_set<Heap*>& visited, size_t& val,
                 const std::function<void(Heap*, size_t&)>& func);
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
        
        /**
         * @brief Assigns a reporter instance to this heap for memory event logging.
         *
         * This method allows injecting a custom implementation of the IReporter interface
         * (e.g., ConsoleReporter, FileReporter). Once set, all subsequent allocations
         * and deallocations performed by this heap will trigger the corresponding
         * callbacks on the reporter.
         *
         * @param reporter Pointer to the reporter instance. Pass nullptr to disable reporting.
         * 
         * @note The Heap does not take ownership of the reporter pointer; the caller is
         * responsible for managing the reporter's lifecycle.
         */
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
         * Updates the total byte count and adds the header to the internal linked list.
         * 
         * @param alloc Pointer to the header of the newly allocated memory.
         */
        void AddAllocation(alloc_header::AllocHeader* alloc);
    
        /**
         * @brief Unregisters an allocation from this heap.
         * Decreases total byte count and removes the header from the internal linked list.
         * 
         * @param alloc Pointer to the header of the memory being freed.
         */
        void RemoveAlloc(alloc_header::AllocHeader* alloc);

        /**
         * @brief Prints all active allocations between two IDs.
         * Used to detect leaks or inspect memory usage between two points in time.
         * 
         * @param bookMark1 The starting Allocation ID (inclusive).
         * @param bookMark2 The ending Allocation ID (inclusive).
         */
        void ReportMemory(int bookMark1, int bookMark2);

        /**
         * @brief Reserves memory for the adjacency list of connected heaps.
         * Use this if you know ahead of time how many heaps will be connected 
         * to this one to avoid reallocations.
         * 
         * @param size The number of heaps to reserve space for.
         */
        void allocateAdjList(size_t size) {
            m_AdjHeaps.reserve(size);
        }

        /**
         * @brief Adds a one-way connection from this heap to another target heap.
         * This adds the target heap to this heap's adjacency list. 
         * For bidirectional linking, use HeapFactory::ConnectHeaps().
         * 
         * @param heap Pointer to the target heap to connect.
         * 
         * @warning [THREAD WARNING] This function acquires a GLOBAL STATIC LOCK on the heap topology.
         * It will block ALL other threads trying to modify heap connections or query hierarchy stats
         * until it completes. It does NOT block standard Alloc/Dealloc on other heaps.
         */
        void AddHeap(Heap* heap);

        /**
         * @brief Calculates the total memory usage for hierarchical heaps.
         * This performs a graph traversal (DFS) to sum the total memory size 
         * of the entire connected component (Cluster) of heaps.
         * 
         * @return size_t Total bytes allocated across the heap graph.
         * 
         * @warning [THREAD WARNING] This function acquires a GLOBAL STATIC LOCK on the heap topology.
         * It will block ALL other threads trying to modify heap connections or query hierarchy stats
         * until it completes. It does NOT block standard Alloc/Dealloc on other heaps.
         * 
         * @warning [PERF WARNING] This uses std::unordered_set for traversal, which triggers
         * dynamic memory allocations on the Default Heap. 
         * DO NOT use this in high-performance loops (e.g., Game Update) or memory stress tests.
         * Intended for Debugging/Snapshots only.
         */
        size_t GetTotalHH();

        /**
         * @brief Counts the total number of active allocations in heap hierarchy.
         * This performs a graph traversal (DFS) to sum the allocation count
         * of the entire connected heaps (Hierarchy) of heaps.
         * @return size_t Total count of allocations across the heap graph.
         * 
         * @warning [THREAD WARNING] This function acquires a GLOBAL STATIC LOCK on the heap topology.
         * It will block ALL other threads trying to modify heap connections or query hierarchy stats
         * until it completes. It does NOT block standard Alloc/Dealloc on other heaps.
         * 
         * @warning [PERF WARNING] This uses std::unordered_set for traversal, which triggers
         * dynamic memory allocations on the Default Heap. 
         * DO NOT use this in high-performance loops (e.g., Game Update) or memory stress tests.
         * Intended for Debugging/Snapshots only.
         */
        size_t CountAllocationsHH();
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

        /**
         * @brief Establishes a bidirectional connection between two heaps.
         * This effectively merges the two heaps into the same `Heap Hierarchy`
         * allowing functions like heap->GetTotalHH() to aggregate data from both.
         * 
         * @param heap1 Pointer to the first heap.
         * @param heap2 Pointer to the second heap.
         * 
         * @warning [THREAD WARNING] This function acquires a GLOBAL STATIC LOCK on the heap topology.
         * It will block ALL other threads trying to modify heap connections or query hierarchy stats
         * until it completes. It does NOT block standard Alloc/Dealloc on other heaps.
         */
        static void ConnectHeaps(Heap* heap1, Heap* heap2){
            if(heap1 && heap2){
                heap1->AddHeap(heap2);
                heap2->AddHeap(heap1);
            }
        }
    };
};