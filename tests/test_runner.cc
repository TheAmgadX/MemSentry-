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

using MEM_SENTRY::heap::Heap;
using MEM_SENTRY::heap::HeapFactory;
using MEM_SENTRY::alloc_header::AllocHeader;

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
};

int main() {
    XTests::run_all();
    return 0;
}