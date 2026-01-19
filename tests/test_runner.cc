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

// Force enable the Memory Sentry system for these tests
#ifndef MEM_SENTRY_ENABLE
#define MEM_SENTRY_ENABLE 1
#endif

#include "mem_sentry/mem_sentry.h"
#include "mem_sentry/heap.h"
#include "mem_sentry/sentry.h"
#include "mem_sentry/alloc_header.h"

using MEM_SENTRY::heap::Heap;
using MEM_SENTRY::heap::HeapFactory;
using MEM_SENTRY::alloc_header::AllocHeader;

//-----------------------------------------------------------------------------
// Helper Macros for Reporting
//-----------------------------------------------------------------------------
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

//-----------------------------------------------------------------------------
// Test Classes
//-----------------------------------------------------------------------------
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

// Aligned structure: 128-byte alignment (larger than standard max_align_t)
struct alignas(128) AlignedDeepData {
    float data[32]; 
};

//-----------------------------------------------------------------------------
// XTests Class
//-----------------------------------------------------------------------------
class XTests {
public:
    static void run_all() {
        std::cout << "=============================================\n";
        std::cout << "    Memory Manager Robust Test Suite\n";
        std::cout << "=============================================\n\n";

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
        TestAlignment(); //! doesn't work now
        TestArrayNewOverrides();
        
        TestThreading(); //! doesn't work now

        TestInteractiveLeakReport();

        std::cout << "\n=============================================\n";
        std::cout << "    \033[32mALL TESTS PASSED SUCCESSFULLY\033[0m\n";
        std::cout << "=============================================\n";
    }

private:
    static void TestBasicAllocation() {
        LOG_TEST("TestBasicAllocation");
        Heap* heap = HeapFactory::GetDefaultHeap();
        int initialCount = heap->CountAllocations();
        long long initialTotal = heap->GetTotal();

        int* p = new int(123);
        
        ASSERT_EQ(heap->CountAllocations(), initialCount + 1);
        ASSERT_EQ(heap->GetTotal(), initialTotal + (long long)sizeof(int));
        ASSERT_EQ(*p, 123);

        delete p;

        ASSERT_EQ(heap->CountAllocations(), initialCount);
        ASSERT_EQ(heap->GetTotal(), initialTotal);
    }

    static void TestArrayAllocation() {
        LOG_TEST("TestArrayAllocation");
        Heap* heap = HeapFactory::GetDefaultHeap();
        int initialCount = heap->CountAllocations();
        long long initialTotal = heap->GetTotal();

        const int ARR_SIZE = 50;
        char* arr = new char[ARR_SIZE];
        
        for(int i=0; i<ARR_SIZE; i++) arr[i] = (char)i;

        ASSERT_EQ(heap->CountAllocations(), initialCount + 1);
        ASSERT_EQ(heap->GetTotal(), initialTotal + ARR_SIZE);

        delete[] arr;

        ASSERT_EQ(heap->CountAllocations(), initialCount);
        ASSERT_EQ(heap->GetTotal(), initialTotal);
    }

    static void TestSentryHeaps() {
        LOG_TEST("TestSentryHeaps");
        Heap physicsHeap("PhysicsHeap");
        Heap audioHeap("AudioHeap");

        PhysicsObject::setHeap(&physicsHeap);
        AudioObject::setHeap(&audioHeap);

        ASSERT_EQ(physicsHeap.CountAllocations(), 0);

        PhysicsObject* p1 = new PhysicsObject();
        PhysicsObject* p2 = new PhysicsObject();
        AudioObject* a1 = new AudioObject();

        ASSERT_EQ(physicsHeap.CountAllocations(), 2);
        ASSERT_EQ(audioHeap.CountAllocations(), 1);

        delete p1;
        delete a1;
        delete p2;
        
        ASSERT_EQ(physicsHeap.CountAllocations(), 0);
        ASSERT_EQ(audioHeap.CountAllocations(), 0);
    }

    static void TestLinkedListIntegrity() {
        LOG_TEST("TestLinkedListIntegrity");
        Heap* heap = HeapFactory::GetDefaultHeap();
        int baseCount = heap->CountAllocations();

        // A -> B -> C
        int* A = new int(1);
        int* B = new int(2);
        int* C = new int(3);

        ASSERT_EQ(heap->CountAllocations(), baseCount + 3);

        delete B; // Remove Middle
        ASSERT_EQ(heap->CountAllocations(), baseCount + 2);

        delete A; // Remove Head
        ASSERT_EQ(heap->CountAllocations(), baseCount + 1);

        delete C; // Remove Tail
        ASSERT_EQ(heap->CountAllocations(), baseCount);
    }

    static void TestStress() {
        LOG_TEST("TestStress");
        Heap* heap = HeapFactory::GetDefaultHeap();
        const int COUNT = 10000;
        
        std::vector<int*> pointers;
        pointers.reserve(COUNT); 

        int startCount = heap->CountAllocations();
        long long startTotal = heap->GetTotal();

        for(int i = 0; i < COUNT; i++) {
            pointers.push_back(new int(i));
        }

        ASSERT_EQ(heap->CountAllocations(), startCount + COUNT);

        // Interleaved deletion
        for(size_t i = 0; i < pointers.size(); i += 2) {
            delete pointers[i];
            pointers[i] = nullptr;
        }

        ASSERT_EQ(heap->CountAllocations(), startCount + (COUNT / 2));

        for(size_t i = 0; i < pointers.size(); i++) {
            if(pointers[i] != nullptr) delete pointers[i];
        }

        ASSERT_EQ(heap->CountAllocations(), startCount);
        ASSERT_EQ(heap->GetTotal(), startTotal);
    }

    static void TestReallocationReuse() {
        LOG_TEST("TestReallocationReuse");
        Heap* heap = HeapFactory::GetDefaultHeap();
        int baseCount = heap->CountAllocations();

        int* p1 = new int(10);
        delete p1;

        int* p2 = new int(20);
        ASSERT_EQ(heap->CountAllocations(), baseCount + 1);
        delete p2;

        ASSERT_EQ(heap->CountAllocations(), baseCount);
    }

    static void TestZeroSizeAllocation() {
        LOG_TEST("TestZeroSizeAllocation");
        Heap* heap = HeapFactory::GetDefaultHeap();
        int startCount = heap->CountAllocations();

        // Standard C++ allows zero size allocs, returns unique ptr
        char* p = new char[0];
        
        ASSERT_TRUE(p != nullptr);
        ASSERT_EQ(heap->CountAllocations(), startCount + 1);
        
        delete[] p;
        ASSERT_EQ(heap->CountAllocations(), startCount);
    }

    static void TestNullPointerDelete() {
        LOG_TEST("TestNullPointerDelete");
        int* p = nullptr;
        delete p; // Should not crash
        ASSERT_TRUE(true);
    }

    static void TestLargeAllocation() {
        LOG_TEST("TestLargeAllocation");
        Heap* heap = HeapFactory::GetDefaultHeap();
        int startCount = heap->CountAllocations();

        const int SIZE = 1024 * 1024; // 1MB
        char* largeBlock = new char[SIZE];
        largeBlock[0] = 'X';
        
        ASSERT_EQ(heap->CountAllocations(), startCount + 1);
        
        delete[] largeBlock;
        ASSERT_EQ(heap->CountAllocations(), startCount);
    }

    static void TestHeapSwitching() {
        LOG_TEST("TestHeapSwitching");
        Heap heapA("HeapA");
        Heap heapB("HeapB");

        PhysicsObject::setHeap(&heapA);
        PhysicsObject* objA = new PhysicsObject(); 

        PhysicsObject::setHeap(&heapB);
        PhysicsObject* objB = new PhysicsObject(); 

        ASSERT_EQ(heapA.CountAllocations(), 1);
        ASSERT_EQ(heapB.CountAllocations(), 1);

        delete objA; // Should be removed from HeapA, despite class static pointing to B
        ASSERT_EQ(heapA.CountAllocations(), 0);
        
        delete objB;
        ASSERT_EQ(heapB.CountAllocations(), 0);
    }

    static void TestAlignment() {
        LOG_TEST("TestAlignment");
        Heap* heap = HeapFactory::GetDefaultHeap();
        int startCount = heap->CountAllocations();

        // 1. Single Aligned Object
        // This triggers operator new(size_t, align_val_t)
        AlignedDeepData* p = new AlignedDeepData();
        
        uintptr_t addr = reinterpret_cast<uintptr_t>(p);
        bool isAligned = (addr % 128 == 0);

        if (!isAligned) {
             std::cout << "[\033[31mFAIL\033[0m] Address " << std::hex << addr 
                       << " is NOT 128-byte aligned." << std::dec << std::endl;
             std::exit(1);
        }

        ASSERT_EQ(heap->CountAllocations(), startCount + 1);

        // 2. Cleanup
        delete p;
        ASSERT_EQ(heap->CountAllocations(), startCount);
    }

    static void TestArrayNewOverrides() {
        LOG_TEST("TestArrayNewOverrides");
        Heap* heap = HeapFactory::GetDefaultHeap();
        int startCount = heap->CountAllocations();

        // Explicitly test new[] logic
        int* arr = new int[10];
        ASSERT_EQ(heap->CountAllocations(), startCount + 1);
        delete[] arr;
        ASSERT_EQ(heap->CountAllocations(), startCount);
    }

    static void TestThreading() {
        LOG_TEST("TestThreading");
        Heap* heap = HeapFactory::GetDefaultHeap();
        
        // This test no longer uses an external mutex because Heap is now thread-safe.
        
        const int NUM_THREADS = 8;
        const int ALLOCS_PER_THREAD = 100;
        
        std::atomic<int> successCount{0};
        std::vector<std::thread> threads;

        int startCount = heap->CountAllocations();

        for(int t=0; t<NUM_THREADS; t++) {
            threads.emplace_back([&]() {
                std::vector<int*> localPtrs;
                
                // Concurrent Allocations
                for(int i=0; i<ALLOCS_PER_THREAD; i++) {
                    localPtrs.push_back(new int(i));
                }
                
                // Concurrent Deallocations
                for(auto ptr : localPtrs) {
                    delete ptr;
                }
                successCount++;
            });
        }

        for(auto& t : threads) {
            t.join();
        }

        ASSERT_EQ(successCount, NUM_THREADS);
        ASSERT_EQ(heap->CountAllocations(), startCount);
    }

    static void TestInteractiveLeakReport() {
        LOG_TEST("TestInteractiveLeakReport");
        Heap* heap = HeapFactory::GetDefaultHeap();
        
        std::cout << "\n\033[36m--- Interactive Leak Check ---\033[0m\n";
        std::cout << "Creating explicit leak of 2 objects (alloc ID X and X+1)...\n";
        
        int* leak1 = new int(111);
        int* leak2 = new int(222);

        // Print report
        heap->ReportMemory(0, 1000000);

        std::cout << "\n\033[33m>> Check console for 'Memory Report'. Press ENTER to cleanup and finish...\033[0m";
        // cin.get() waits for user input
        std::cin.get(); 

        delete leak1;
        delete leak2;
        
        std::cout << "Cleaned up.\n";
    }
};

int main() {
    XTests::run_all();
    return 0;
}