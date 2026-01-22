#include <iostream>
#include <vector>
#include <cassert>
#include <string>
#include <algorithm>
#include <thread>
#include <random>
#include <atomic>
#include <cstdint>
#include <new>      
#include <mutex>
#include <limits>

// ----------------------------------------------------------------------------
// CONFIGURATION
// ----------------------------------------------------------------------------
#include "mem_sentry/mem_sentry.h"
#include "mem_sentry/heap.h"
#include "mem_sentry/sentry.h"
#include "mem_sentry/alloc_header.h"

#include "mem_sentry/reporter.h"

using MEM_SENTRY::heap::Heap;
using MEM_SENTRY::heap::HeapFactory;
using MEM_SENTRY::alloc_header::AllocHeader;

static MEM_SENTRY::reporter::ConsoleReporter gConsoleReporter;

// ----------------------------------------------------------------------------
// HELPER MACROS
// ----------------------------------------------------------------------------
#define ASSERT_EQ(val, expected) \
    do { \
        if((val) != (expected)) { \
            std::cerr << "[\033[31mFAIL\033[0m] " << __FUNCTION__ << " line " << __LINE__ \
                      << ": Expected " << #val << " == " << expected \
                      << ", but got " << (val) << "\n"; \
            std::exit(1); \
        } \
    } while(0)

#define ASSERT_TRUE(cond) \
    do { \
        if(!(cond)) { \
            std::cerr << "[\033[31mFAIL\033[0m] " << __FUNCTION__ << " line " << __LINE__ \
                      << ": Assertion " << #cond << " failed.\n"; \
            std::exit(1); \
        } \
    } while(0)

#define LOG_TEST(name) std::cout << "[\033[32mRUN\033[0m] " << name << "..." << std::endl

// ----------------------------------------------------------------------------
// TEST OBJECTS
// ----------------------------------------------------------------------------
class PhysicsObject : public MEM_SENTRY::sentry::ISentry<PhysicsObject> {
public:
    double x, y, z;
    PhysicsObject() : x(0), y(0), z(0) {}
};

class AudioObject : public MEM_SENTRY::sentry::ISentry<AudioObject> {
public:
    int sampleRate;
    AudioObject() : sampleRate(44100) {}
};

// Aligned structure: 128-byte alignment
struct alignas(128) AlignedDeepData {
    float data[32]; 
};

// ----------------------------------------------------------------------------
// TEST SUITE
// ----------------------------------------------------------------------------
class XTests {
public:
    static void run_all() {
        std::cout << "=============================================\n";
        std::cout << "    Memory Sentry Full Robust Test Suite\n";
        std::cout << "    Mode: " << (MEM_SENTRY_ENABLE ? "\033[36mENABLED\033[0m" : "\033[33mDISABLED (Std Malloc)\033[0m") << "\n";
        std::cout << "=============================================\n\n";

        // --- Original Tests ---
        TestBasicAllocation();
        TestArrayAllocation();
        TestSentryHeaps();
        TestLinkedListIntegrity();
        TestStress();
        TestReallocationReuse();
        TestZeroSizeAllocation();
        TestNullPointerDelete();
        TestLargeAllocation();
        TestHeapSwitching();
        TestAlignment();
        TestArrayNewOverrides();
        
        // --- Enhanced Coverage Tests (NEW) ---
        TestNothrowNew();             // UPDATED: Now checks Aligned Nothrow paths
        TestAlignedArrayNew();
        TestPlacementNew();
        TestHugeAlignment();
        
        // --- Edge Case Operators (NEW) ---
        TestDirectHeapOperator();     // Checks new (heap_ptr) T(...)
        TestExplicitOperatorCalls();  // Checks explicit delete(p, size) calls

        // --- Additional operator coverage (ADDED) ---
        TestAlignedHeapOperator();    // new(alignment, Heap*) T(...)
        TestNothrowAlignedPlacement(); // new(alignment, nothrow) variants
        TestOperatorDeleteNothrowExplicit(); // explicit nothrow delete call
        // --- Reporting ---
        TestInteractiveLeakReport();

        TestMultiThreadedAllocations();

        TestHeapHierarchy();
        TestHeapHierarchyThreadSafety();

        std::cout << "\n=============================================\n";
        std::cout << "    \033[32mALL TESTS PASSED SUCCESSFULLY\033[0m\n";
        std::cout << "=============================================\n";
    }

private:
    static size_t GetCount(Heap* h) {
#if MEM_SENTRY_ENABLE
        return h->CountAllocations();
#else
        return 0;
#endif
    }

    static long long GetTotal(Heap* h) {
#if MEM_SENTRY_ENABLE
        return h->GetTotal();
#else
        return 0;
#endif
    }

    // ------------------------------------------------------------------------
    // CORE TESTS
    // ------------------------------------------------------------------------

    static void TestBasicAllocation() {
        LOG_TEST("TestBasicAllocation");
        Heap* heap = HeapFactory::GetDefaultHeap();
        size_t initialCount = GetCount(heap);
        long long initialTotal = GetTotal(heap);

        int* p = new int(123);
        
        #if MEM_SENTRY_ENABLE
        ASSERT_EQ(GetCount(heap), initialCount + 1);
        ASSERT_EQ(GetTotal(heap), initialTotal + (long long)sizeof(int));
        #endif
        ASSERT_EQ(*p, 123);

        delete p;

        ASSERT_EQ(GetCount(heap), initialCount);
        ASSERT_EQ(GetTotal(heap), initialTotal);
    }

    static void TestArrayAllocation() {
        LOG_TEST("TestArrayAllocation");
        Heap* heap = HeapFactory::GetDefaultHeap();
        size_t initialCount = GetCount(heap);

        const int ARR_SIZE = 50;
        char* arr = new char[ARR_SIZE];
        
        for(int i=0; i<ARR_SIZE; i++) arr[i] = (char)i;

        #if MEM_SENTRY_ENABLE
        ASSERT_EQ(GetCount(heap), initialCount + 1);
        #endif

        delete[] arr;

        ASSERT_EQ(GetCount(heap), initialCount);
    }

    static void TestSentryHeaps() {
        LOG_TEST("TestSentryHeaps (ISentry)");
        Heap physicsHeap("PhysicsHeap");
        Heap audioHeap("AudioHeap");
        physicsHeap.SetReporter(&gConsoleReporter);
        audioHeap.SetReporter(&gConsoleReporter);

        PhysicsObject::setHeap(&physicsHeap);
        AudioObject::setHeap(&audioHeap);

        size_t startP = GetCount(&physicsHeap);

        PhysicsObject* p1 = new PhysicsObject();
        PhysicsObject* p2 = new PhysicsObject();
        AudioObject* a1 = new AudioObject();

        #if MEM_SENTRY_ENABLE
        ASSERT_EQ(GetCount(&physicsHeap), startP + 2);
        ASSERT_EQ(GetCount(&audioHeap), 1);
        #endif

        delete p1;
        delete a1;
        delete p2;
        
        ASSERT_EQ(GetCount(&physicsHeap), startP);
        ASSERT_EQ(GetCount(&audioHeap), 0);
    }

    static void TestLinkedListIntegrity() {
        LOG_TEST("TestLinkedListIntegrity");
        Heap* heap = HeapFactory::GetDefaultHeap();
        size_t baseCount = GetCount(heap);

        int* A = new int(1);
        int* B = new int(2);
        int* C = new int(3);

        #if MEM_SENTRY_ENABLE
        ASSERT_EQ(GetCount(heap), baseCount + 3);
        #endif

        delete B; 
        #if MEM_SENTRY_ENABLE
        ASSERT_EQ(GetCount(heap), baseCount + 2);
        #endif

        delete A; 
        #if MEM_SENTRY_ENABLE
        ASSERT_EQ(GetCount(heap), baseCount + 1);
        #endif

        delete C;
        ASSERT_EQ(GetCount(heap), baseCount);
    }

    static void TestStress() {
        LOG_TEST("TestStress");
        Heap* heap = HeapFactory::GetDefaultHeap();
        const int COUNT = 5000;
        
        std::vector<int*> pointers;
        pointers.reserve(COUNT); 

        size_t startCount = GetCount(heap);

        for(int i = 0; i < COUNT; i++) {
            pointers.push_back(new int(i));
        }

        #if MEM_SENTRY_ENABLE
        ASSERT_EQ(GetCount(heap), startCount + COUNT);
        #endif

        for(size_t i = 0; i < pointers.size(); i += 2) {
            delete pointers[i];
            pointers[i] = nullptr;
        }

        #if MEM_SENTRY_ENABLE
        ASSERT_EQ(GetCount(heap), startCount + (COUNT / 2));
        #endif

        for(size_t i = 0; i < pointers.size(); i++) {
            if(pointers[i] != nullptr) delete pointers[i];
        }

        ASSERT_EQ(GetCount(heap), startCount);
    }

    static void TestReallocationReuse() {
        LOG_TEST("TestReallocationReuse");
        Heap* heap = HeapFactory::GetDefaultHeap();
        size_t baseCount = GetCount(heap);

        int* p1 = new int(10);
        delete p1;

        int* p2 = new int(20);
        #if MEM_SENTRY_ENABLE
        ASSERT_EQ(GetCount(heap), baseCount + 1);
        #endif
        delete p2;

        ASSERT_EQ(GetCount(heap), baseCount);
    }

    static void TestZeroSizeAllocation() {
        LOG_TEST("TestZeroSizeAllocation");
        Heap* heap = HeapFactory::GetDefaultHeap();
        size_t startCount = GetCount(heap);

        char* p = new char[0];
        
        ASSERT_TRUE(p != nullptr);
        #if MEM_SENTRY_ENABLE
        ASSERT_EQ(GetCount(heap), startCount + 1);
        #endif
        
        delete[] p;
        ASSERT_EQ(GetCount(heap), startCount);
    }

    static void TestNullPointerDelete() {
        LOG_TEST("TestNullPointerDelete");
        int* p = nullptr;
        delete p; 
        ASSERT_TRUE(true);
    }

    static void TestLargeAllocation() {
        LOG_TEST("TestLargeAllocation");
        Heap* heap = HeapFactory::GetDefaultHeap();
        size_t startCount = GetCount(heap);

        const int SIZE = 1024 * 1024; // 1MB
        char* largeBlock = new char[SIZE];
        largeBlock[0] = 'X';
        
        #if MEM_SENTRY_ENABLE
        ASSERT_EQ(GetCount(heap), startCount + 1);
        #endif
        
        delete[] largeBlock;
        ASSERT_EQ(GetCount(heap), startCount);
    }

    static void TestHeapSwitching() {
        LOG_TEST("TestHeapSwitching");
        Heap heapA("HeapA");
        Heap heapB("HeapB");
        heapA.SetReporter(&gConsoleReporter);
        heapB.SetReporter(&gConsoleReporter);

        PhysicsObject::setHeap(&heapA);
        PhysicsObject* objA = new PhysicsObject(); 

        PhysicsObject::setHeap(&heapB);
        PhysicsObject* objB = new PhysicsObject(); 

        #if MEM_SENTRY_ENABLE
        ASSERT_EQ(GetCount(&heapA), 1);
        ASSERT_EQ(GetCount(&heapB), 1);
        #endif

        delete objA; 
        #if MEM_SENTRY_ENABLE
        ASSERT_EQ(GetCount(&heapA), 0);
        #endif
        
        delete objB;
        #if MEM_SENTRY_ENABLE
        ASSERT_EQ(GetCount(&heapB), 0);
        #endif
    }

    static void TestAlignment() {
        LOG_TEST("TestAlignment (Scalar)");
        Heap* heap = HeapFactory::GetDefaultHeap();
        size_t startCount = GetCount(heap);

        AlignedDeepData* p = new AlignedDeepData();
        
        uintptr_t addr = reinterpret_cast<uintptr_t>(p);
        bool isAligned = (addr % 128 == 0);
        ASSERT_TRUE(isAligned);

        #if MEM_SENTRY_ENABLE
        ASSERT_EQ(GetCount(heap), startCount + 1);
        #endif

        delete p;
        ASSERT_EQ(GetCount(heap), startCount);
    }

    static void TestArrayNewOverrides() {
        LOG_TEST("TestArrayNewOverrides");
        Heap* heap = HeapFactory::GetDefaultHeap();
        size_t startCount = GetCount(heap);

        int* arr = new int[10];
        #if MEM_SENTRY_ENABLE
        ASSERT_EQ(GetCount(heap), startCount + 1);
        #endif
        delete[] arr;
        ASSERT_EQ(GetCount(heap), startCount);
    }

    // ------------------------------------------------------------------------
    // ENHANCED / NEW OPERATOR TESTS
    // ------------------------------------------------------------------------

    static void TestNothrowNew() {
        LOG_TEST("TestNothrowNew (Std + Aligned)");
        Heap* heap = HeapFactory::GetDefaultHeap();
        size_t start = GetCount(heap);

        // 1. Scalar Nothrow
        int* p1 = new (std::nothrow) int(42);
        ASSERT_TRUE(p1 != nullptr);
        
        // 2. Array Nothrow
        int* p2 = new (std::nothrow) int[10];
        ASSERT_TRUE(p2 != nullptr);

        // 3. Aligned Scalar Nothrow (NEW)
        // This exercises: operator new(size_t, align_val_t, nothrow_t)
        AlignedDeepData* p3 = new (std::nothrow) AlignedDeepData();
        ASSERT_TRUE(p3 != nullptr);
        uintptr_t addr3 = reinterpret_cast<uintptr_t>(p3);
        ASSERT_TRUE(addr3 % 128 == 0);

        // 4. Aligned Array Nothrow (NEW)
        // This exercises: operator new[](size_t, align_val_t, nothrow_t)
        AlignedDeepData* p4 = new (std::nothrow) AlignedDeepData[2];
        ASSERT_TRUE(p4 != nullptr);
        uintptr_t addr4 = reinterpret_cast<uintptr_t>(p4);
        ASSERT_TRUE(addr4 % 128 == 0);

        #if MEM_SENTRY_ENABLE
        ASSERT_EQ(GetCount(heap), start + 4);
        #endif

        delete p1;
        delete[] p2;
        delete p3;
        delete[] p4;

        ASSERT_EQ(GetCount(heap), start);
    }

    static void TestAlignedArrayNew() {
        LOG_TEST("TestAlignedArrayNew");
        Heap* heap = HeapFactory::GetDefaultHeap();
        size_t start = GetCount(heap);

        AlignedDeepData* pArr = new AlignedDeepData[2];
        uintptr_t addr = reinterpret_cast<uintptr_t>(pArr);
        ASSERT_TRUE(addr % 128 == 0);

        #if MEM_SENTRY_ENABLE
        ASSERT_EQ(GetCount(heap), start + 1);
        #endif

        delete[] pArr;
        ASSERT_EQ(GetCount(heap), start);
    }

    static void TestPlacementNew() {
        LOG_TEST("TestPlacementNew (Should Bypass Tracker)");
        Heap* heap = HeapFactory::GetDefaultHeap();
        size_t start = GetCount(heap);

        char buffer[sizeof(int)];
        int* p = new (buffer) int(99);

        ASSERT_EQ(*p, 99);
        // Placement new must NOT allocate memory
        ASSERT_EQ(GetCount(heap), start);
    }

    static void TestHugeAlignment() {
        LOG_TEST("TestHugeAlignment (4096 bytes)");
        Heap* heap = HeapFactory::GetDefaultHeap();
        size_t start = GetCount(heap);

        void* ptr = ::operator new(1024, std::align_val_t(4096));
        uintptr_t addr = reinterpret_cast<uintptr_t>(ptr);
        ASSERT_TRUE(addr % 4096 == 0);

        #if MEM_SENTRY_ENABLE
        ASSERT_EQ(GetCount(heap), start + 1);
        #endif

        ::operator delete(ptr, std::align_val_t(4096));
        ASSERT_EQ(GetCount(heap), start);
    }

    // NEW: Tests operator new(size_t, Heap*) directly
    static void TestDirectHeapOperator() {
        LOG_TEST("TestDirectHeapOperator (new(heapPtr) T)");
        Heap explicitHeap("ExplicitHeap");
        explicitHeap.SetReporter(&gConsoleReporter);
        
        // This validates the override: operator new(size_t, MEM_SENTRY::heap::Heap*)
        int* p = new (&explicitHeap) int(555);

        ASSERT_EQ(*p, 555);

        #if MEM_SENTRY_ENABLE
        ASSERT_EQ(GetCount(&explicitHeap), 1);
        // Default heap should remain untouched
        ASSERT_EQ(GetCount(HeapFactory::GetDefaultHeap()), 0);
        #endif

        delete p;

        #if MEM_SENTRY_ENABLE
        ASSERT_EQ(GetCount(&explicitHeap), 0);
        #endif
    }

    // NEW: Tests explicit calls to ensure linkage is correct
    static void TestExplicitOperatorCalls() {
        LOG_TEST("TestExplicitOperatorCalls");
        Heap* heap = HeapFactory::GetDefaultHeap();
        size_t start = GetCount(heap);

        // 1. Sized Delete (Often optimized out, but must link)
        int* p1 = new int(1);
        ::operator delete(p1, sizeof(int));
        
        // 2. Sized Aligned Delete
        void* p2 = ::operator new(128, std::align_val_t(64));
        ::operator delete(p2, 128, std::align_val_t(64));

        ASSERT_EQ(GetCount(heap), start);
    }

    static void TestAlignedHeapOperator() {
    LOG_TEST("TestAlignedHeapOperator (new(alignment, Heap*))");
    Heap explicitHeap("ExplicitAlignedHeap");
    explicitHeap.SetReporter(&gConsoleReporter);

    size_t start = GetCount(&explicitHeap);

    AlignedDeepData* p = new (std::align_val_t(128), &explicitHeap) AlignedDeepData();
    ASSERT_TRUE(p != nullptr);
    uintptr_t addr = reinterpret_cast<uintptr_t>(p);
    ASSERT_TRUE(addr % 128 == 0);

#if MEM_SENTRY_ENABLE
    ASSERT_EQ(GetCount(&explicitHeap), start + 1);
    ASSERT_EQ(GetCount(HeapFactory::GetDefaultHeap()), 0);
#endif

    delete p;

#if MEM_SENTRY_ENABLE
    ASSERT_EQ(GetCount(&explicitHeap), start);
#endif
    }

    static void TestNothrowAlignedPlacement() {
    LOG_TEST("TestNothrowAlignedPlacement (aligned nothrow new)");
    Heap* heap = HeapFactory::GetDefaultHeap();
    size_t start = GetCount(heap);

    AlignedDeepData* p = new (std::align_val_t(128), std::nothrow) AlignedDeepData();
    ASSERT_TRUE(p != nullptr);
    uintptr_t addr = reinterpret_cast<uintptr_t>(p);
    ASSERT_TRUE(addr % 128 == 0);

#if MEM_SENTRY_ENABLE
    ASSERT_EQ(GetCount(heap), start + 1);
#endif

    delete p;
    ASSERT_EQ(GetCount(heap), start);
    }

    static void TestOperatorDeleteNothrowExplicit() {
    LOG_TEST("TestOperatorDeleteNothrowExplicit");
    Heap* heap = HeapFactory::GetDefaultHeap();
    size_t start = GetCount(heap);

    int* p = new (std::nothrow) int(7);
    ASSERT_TRUE(p != nullptr);

#if MEM_SENTRY_ENABLE
    ASSERT_EQ(GetCount(heap), start + 1);
#endif

    ::operator delete(p, std::nothrow);

    ASSERT_EQ(GetCount(heap), start);
    }

    static void TestInteractiveLeakReport() {
        LOG_TEST("TestInteractiveLeakReport");
        
        #if !MEM_SENTRY_ENABLE
        std::cout << "Skipping Report Test (Sentry Disabled)\n";
        return;
        #endif

        Heap* heap = HeapFactory::GetDefaultHeap();
        std::cout << "\n\033[36m--- Interactive Leak Check ---\033[0m\n";
        std::cout << "Creating explicit leaks: \n";
        std::cout << "1. Standard int\n2. Aligned Object (128 bytes)\n3. Char Array\n";

        int* leak1 = new int(111);
        AlignedDeepData* leak2 = new AlignedDeepData();
        char* leak3 = new char[64];

        heap->ReportMemory(0, 1000000);

        std::cout << "\n\033[33m>> Check console for 'Memory Report' containing 3 distinct blocks.\n";
        std::cout << ">> Press ENTER to clean up and finish tests...\033[0m";
        std::cin.get(); 

        delete leak1;
        delete leak2;
        delete[] leak3;
        
        ASSERT_EQ(GetCount(heap), 0);
        std::cout << "Cleaned up.\n";
    }

    static void TestMultiThreadedAllocations() {
        LOG_TEST("TestMultiThreadedAllocations (Stress Test)");
        Heap* heap = HeapFactory::GetDefaultHeap();
        size_t startCount = GetCount(heap);
        long long startTotal = GetTotal(heap);
        
        const int NUM_THREADS = 10;
        const int ALLOCS_PER_THREAD = 1000;
        
        { // <- begin scope
            std::vector<std::thread> threads;
            std::atomic<bool> threadError(false);
            // Lambda for worker threads
            auto worker = [&]() {
                std::vector<int*> ptrs;
                ptrs.reserve(ALLOCS_PER_THREAD);
    
                try {
                    // Phase 1: Rapid Allocation
                    for (int i = 0; i < ALLOCS_PER_THREAD; ++i) {
                        ptrs.push_back(new int(i));
                    }
    
                    // Phase 2: Rapid Deallocation (in random order if we wanted, but LIFO is fine for stress)
                    for (int* p : ptrs) {
                        delete p;
                    }
                } catch (...) {
                    threadError = true;
                }
            };
    
            // Launch threads
            for (int i = 0; i < NUM_THREADS; ++i) {
                threads.emplace_back(worker);
            }
    
            // Wait for all threads to finish
            for (auto& t : threads) {
                if (t.joinable()) t.join();
            }

            ASSERT_TRUE(!threadError);
        } // <--- END SCOPE: 'threads' vector is destroyed here.

        // Verify Heap Integrity
        // If locks failed, the linked list would be corrupted, likely causing a crash before this point.
        // If locks worked but logic was flawed, counts wouldn't match.
        
        #if MEM_SENTRY_ENABLE
        // Ensure we are back to exactly where we started
        size_t finalCount = GetCount(heap);
        long long finalTotal = GetTotal(heap);

        if (finalCount != startCount) {
             std::cerr << "Thread Race Detected! Expected Count: " << startCount 
                       << " Got: " << finalCount << "\n";
        }
        ASSERT_EQ(finalCount, startCount);
        ASSERT_EQ(finalTotal, startTotal);
        #endif
    }

    static void TestHeapHierarchy() {
        LOG_TEST("TestHeapHierarchy (Graph Logic)");
        
        // 1. Create a cluster of heaps
        Heap root("Root");
        Heap childA("ChildA");
        Heap childB("ChildB");
        Heap isolated("Isolated");

        root.SetReporter(&gConsoleReporter);
        childA.SetReporter(&gConsoleReporter);
        childB.SetReporter(&gConsoleReporter);

        // 2. Allocate data distributed across them
        // using the placement new syntax: new (&heap) Type(...)
        int* pRoot  = new (&root) int(1);
        int* pA     = new (&childA) int(2);
        int* pB     = new (&childB) int(3);
        int* pIso   = new (&isolated) int(4);

        // 3. Build the Hierarchy
        // Graph: ChildA <--> Root --> ChildB
        HeapFactory::ConnectHeaps(&root, &childA); // Bidirectional
        root.AddHeap(&childB);              // Directional (Root -> ChildB)

        #if MEM_SENTRY_ENABLE
        // 4. Verify Root sees everyone (Root + A + B = 3 allocations)
        // DFS should visit Root, then A, then B.
        size_t totalAlloc = root.CountAllocationsHH();
        ASSERT_EQ(totalAlloc, 3);
        
        // Verify Memory Sum (3 * sizeof(int))
        size_t totalBytes = root.GetTotalHH();
        ASSERT_EQ(totalBytes, 3 * sizeof(int));

        // 5. Verify ChildA sees Root and ChildB (because A -> Root -> B)
        // Since it's bidirectional (A <-> Root), A can reach B through Root.
        ASSERT_EQ(childA.CountAllocationsHH(), 3);

        // 6. Verify ChildB sees NOTHING extra (because Root->ChildB is one-way)
        // ChildB has no outgoing connections.
        ASSERT_EQ(childB.CountAllocationsHH(), 1);

        // 7. Verify Isolated heap is alone
        ASSERT_EQ(isolated.CountAllocationsHH(), 1);
        #endif

        // Cleanup
        delete pRoot;
        delete pA;
        delete pB;
        delete pIso;
    }

    static void TestHeapHierarchyThreadSafety() {
        LOG_TEST("TestHeapHierarchyThreadSafety (Deadlock Check)");
        
        Heap heapA("ThreadHeapA");
        Heap heapB("ThreadHeapB");

        // Attach Reporter so you can SEE the allocations
        heapA.SetReporter(&gConsoleReporter);
        heapB.SetReporter(&gConsoleReporter);
        
        // Connect them (A <-> B)
        HeapFactory::ConnectHeaps(&heapA, &heapB); 

        std::atomic<bool> running(true);
        std::atomic<size_t> maxObservedCount(0);

        // --- WORKER THREAD ---
        // 1. Allocates a batch (fills the heap)
        // 2. Deletes the batch (empties the heap)
        std::thread worker([&]() {
            // Pre-allocate vector storage to avoid noise from the vector itself
            std::vector<int*> ptrs;
            ptrs.reserve(100); 

            while(running) {
                // FILL: This will trigger 50 "Alloc" logs
                for(int i = 0; i < 50; i++) {
                    ptrs.push_back(new (&heapA) int(42));
                }

                // DRAIN: This will trigger 50 "Dealloc" logs
                for(auto p : ptrs) {
                    delete p;
                }
                ptrs.clear();

                // Yield to let the Observer thread sneak in
                std::this_thread::yield();
            }
        });

        // --- OBSERVER THREAD ---
        // Tries to traverse the graph while the worker is spamming Alloc/Dealloc
        std::thread observer([&]() {
            while(running) {
                #if MEM_SENTRY_ENABLE
                // This locks the Graph Topology (Static Mutex)
                size_t count = heapA.CountAllocationsHH();
                
                size_t currentMax = maxObservedCount.load();
                if(count > currentMax) {
                    maxObservedCount.store(count);
                }
                #else
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
                #endif
            }
        });

        // Run for 100ms
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        running = false;
        worker.join();
        observer.join();

        #if MEM_SENTRY_ENABLE
        // 1. Integrity Check: Heap must be empty at the end
        ASSERT_EQ(heapA.CountAllocations(), 0);
        ASSERT_EQ(heapB.CountAllocations(), 0);

        // 2. Liveness Check: Did we actually see the heap growing?
        std::cout << "Max Concurrent Allocations Observed: " << maxObservedCount << "\n";
        ASSERT_TRUE(maxObservedCount > 0);
        #endif
    }
};

int main() {
    // Assign console reporter to default heap so tests can log via interface
    HeapFactory::GetDefaultHeap()->SetReporter(&gConsoleReporter);

    XTests::run_all();
    return 0;
}