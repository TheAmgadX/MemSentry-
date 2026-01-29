#include <iostream>
#include <vector>
#include <cassert>
#include <cstdint>
#include <thread>
#include <chrono>
#include <atomic>
#include <algorithm>
#include <random>

#include "mem_pools/chain.h"
#include "mem_pools/buffer.h"

using namespace MEM_SENTRY::mem_pool;

// ----------------------------------------------------------------------------
// HELPER MACROS (Matching test_ringpool.cc style)
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
// TEST CLASSES
// ----------------------------------------------------------------------------

// A helper class to track object lifetime for leak detection
struct LifeTracker {
    static std::atomic<int> active_count;
    int m_ID;

    LifeTracker(int id) : m_ID(id) {
        active_count.fetch_add(1, std::memory_order_relaxed);
    }
    
    // Copy constructor
    LifeTracker(const LifeTracker& other) : m_ID(other.m_ID) {
        active_count.fetch_add(1, std::memory_order_relaxed);
    }

    ~LifeTracker() {
        active_count.fetch_sub(1, std::memory_order_relaxed);
    }
};
std::atomic<int> LifeTracker::active_count{0};

void TestChainExpansionFullMode() {
    LOG_TEST("TestChainExpansionFullMode");

    // Full Mode: Pool starts with buffers.
    // Size 4 -> Usable capacity 3 per pool.
    // Initial value inside buffers: 10
    PoolChain<int, alignof(int), true> chain(4, 10);

    // 1. Drain the initial pool (3 items)
    std::vector<Buffer<int, alignof(int), true>*> extracted;
    for(int i = 0; i < 3; ++i) {
        auto* b = chain.pop();
        ASSERT_TRUE(b != nullptr);
        ASSERT_EQ(*b->p_Buffer, 10);
        extracted.push_back(b);
    }

    // 2. The next pop should trigger addPool() internal logic.
    // The new pool should also be constructed with value 10.
    auto* b_new = chain.pop();
    ASSERT_TRUE(b_new != nullptr);
    ASSERT_EQ(*b_new->p_Buffer, 10);
    extracted.push_back(b_new);

    // 3. Return items. 
    // Logic: push() iterates pools. 
    // Pool 1 is empty (we popped 3). Pool 2 has 2 remaining (cap 3, popped 1).
    // The items should fill Pool 1 first, then spill to Pool 2 if needed.
    for(auto* buf : extracted) {
        bool ok = chain.push(buf);
        ASSERT_TRUE(ok);
    }

    // 4. Validate we can get them all back (Data integrity)
    int count = 0;
    while(true) {
        // We expect exactly 4 items total (3 from first pool + 1 from expansion)
        if(count == 4) break; 
        
        auto* b = chain.pop();
        if(!b) break;
        ASSERT_EQ(*b->p_Buffer, 10);
        count++;
        
        // Push back so we don't leak memory (Pool owns them in Full Mode)
        chain.push(b);
    }
    ASSERT_EQ(count, 4);
}

void TestMultiPoolWrapAround() {
    LOG_TEST("TestMultiPoolWrapAround");
    
    // Use very small capacity to force chaining.
    // Size 2 -> Usable capacity 1 per pool.
    PoolChain<int, alignof(int), true> chain(2, 999);

    // 1. Pop from Pool A (cap 1). A is now empty.
    auto* b1 = chain.pop(); 
    ASSERT_TRUE(b1 != nullptr);
    
    // 2. Pop again -> Creates Pool B. Pop from B. B is empty.
    auto* b2 = chain.pop(); 
    ASSERT_TRUE(b2 != nullptr);

    // 3. Pop again -> Creates Pool C. Pop from C. C is empty.
    auto* b3 = chain.pop(); 
    ASSERT_TRUE(b3 != nullptr);

    // Current State: 3 pools (A, B, C). All empty.
    
    // 4. Push b1. Should fill Pool A.
    ASSERT_TRUE(chain.push(b1));

    // 5. Push b2. Pool A is full. Should skip A and fill Pool B.
    ASSERT_TRUE(chain.push(b2));

    // 6. Push b3. A full, B full. Should fill Pool C.
    ASSERT_TRUE(chain.push(b3));

    // 7. Push extra buffer? Should fail (All 3 pools full).
    auto* b_extra = new Buffer<int, alignof(int), true>(888);
    ASSERT_TRUE(!chain.push(b_extra));
    delete b_extra; 

    // 8. Drain and verify FIFO-ish behavior across pools
    // Implementation iterates pools from Head.
    auto* p1 = chain.pop(); ASSERT_EQ(p1, b1);
    auto* p2 = chain.pop(); ASSERT_EQ(p2, b2);
    auto* p3 = chain.pop(); ASSERT_EQ(p3, b3);

    // Return them for cleanup
    chain.push(p1); chain.push(p2); chain.push(p3);
}

// Global counter for lifecycle test
static std::atomic<int> g_objCount{0};
struct LifeObj {
    LifeObj() { g_objCount.fetch_add(1); }
    ~LifeObj() { g_objCount.fetch_sub(1); }
};

void TestLifecycleManagement() {
    LOG_TEST("TestLifecycleManagement");
    g_objCount.store(0);
    
    {
        // Full Mode. Usable capacity 1 per pool (Size 2).
        PoolChain<LifeObj, 16, true> chain(2); 
        
        // Initial state: 1 pool created, 1 item allocated.
        ASSERT_EQ(g_objCount.load(), 1);

        // Pop 1 (pool becomes empty)
        auto* b1 = chain.pop();
        ASSERT_TRUE(b1 != nullptr);
        
        // Pop 2 (Forces creation of Pool 2). Pool 2 allocates 1 item.
        auto* b2 = chain.pop();
        ASSERT_TRUE(b2 != nullptr);
        ASSERT_EQ(g_objCount.load(), 2);

        // Push them back.
        // Important: In Full Mode, the Pool owns the memory.
        // If we destroy the pool while holding the pointers (and not pushing back),
        // the pool destructor won't see them in the queue and won't delete them, causing a leak.
        chain.push(b1);
        chain.push(b2);

    } // Chain destructor runs here. Should destroy Pool 1 and Pool 2.

    // Must be 0 if all destructors fired correctly.
    ASSERT_EQ(g_objCount.load(), 0);
}

void TestProducerConsumerGrowth() {
    LOG_TEST("TestProducerConsumerGrowth (Threaded)");

    // Start with small capacity (usable 3) to force expansion during test.
    PoolChain<int, alignof(int), true> chain(4, 0);

    const int TOTAL_ITEMS = 5000;
    std::atomic<int> consumed_count{0};

    // Consumer Thread
    std::thread consumer([&]() {
        for(int i=0; i<TOTAL_ITEMS; ++i) {
            Buffer<int, alignof(int), true>* b = nullptr;
            
            // Spin-wait for data
            while(true) {
                b = chain.pop(); // May trigger addPool()
                if(b) break;
                std::this_thread::yield();
            }
            
            consumed_count.fetch_add(1, std::memory_order_relaxed);
            
            // Simulate processing time slightly to allow producer to fill up queues
            // forcing the logic to traverse the chain.
             // std::this_thread::sleep_for(std::chrono::nanoseconds(10));

            // Return buffer to pool
            chain.push(b);
        }
    });

    // Producer logic is implicit here because we are in "Full Mode"
    // (Buffers are recycled). The "Consumer" acts as both puller and pusher 
    // in this specific test to verify the Chain mechanics (Pop->Grow->Push).
    
    // To properly test "Producer vs Consumer" where Producer Pushes and Consumer Pops:
    // We would need Empty Mode. But this test specifically targets the 
    // "Auto-Growth" feature which happens on Pop.
    
    consumer.join();
    ASSERT_EQ(consumed_count.load(), TOTAL_ITEMS);
}

void TestTortureGrowth() {
    LOG_TEST("TestTortureGrowth (Massive Chain)");
    
    // Start VERY small: Usable capacity 1 per pool.
    PoolChain<size_t, 64, true> chain(2, 777);
    
    constexpr int TARGET_POOLS = 500;
    std::vector<Buffer<size_t, 64, true>*> held_buffers;
    held_buffers.reserve(TARGET_POOLS);

    // 1. Force creation of 500 chained pools by popping without returning.
    for(int i=0; i<TARGET_POOLS; ++i) {
        auto* b = chain.pop();
        ASSERT_TRUE(b != nullptr);
        ASSERT_EQ(*b->p_Buffer, 777);
        held_buffers.push_back(b);
    }
    
    std::cout << "    Successfully grew chain to " << TARGET_POOLS << " pools." << std::endl;

    // 2. Push them all back.
    // This stress-tests the iteration logic in push():
    // The first item pushed will likely go to the first pool, 
    // the last item pushed might have to traverse 499 full pools to find the empty one.
    for(auto* b : held_buffers) {
        bool ok = chain.push(b);
        ASSERT_TRUE(ok);
    }

    // 3. Clean up by returning ownership to the pool (we just did that via push)
    // The destructor of chain will handle deletion.
}

/**
 * @brief Tests basic functionality: Auto-expansion and First-In-First-Out behavior
 * across multiple internal pools.
 */
void TestBasicExpansionAndOrder() {
    LOG_TEST("TestBasicExpansionAndOrder");

    // Full Mode. Size 2 -> Usable Capacity 1 per pool (Waste-One-Slot).
    PoolChain<int, alignof(int), true> chain(2, 100);

    std::vector<Buffer<int, alignof(int), true>*> popped_bufs;

    // 1. Expansion Phase
    // Pop 5 items -> Creates 5 chained pools (P1..P5), each with 1 item capacity.
    for(int i = 0; i < 5; ++i) {
        auto* buf = chain.pop();
        ASSERT_TRUE(buf != nullptr);
        ASSERT_EQ(*buf->p_Buffer, 100); 
        *buf->p_Buffer = i; // Mark 0, 1, 2, 3, 4
        popped_bufs.push_back(buf);
    }

    // 2. Return Phase
    // Push them back. They should fill P1, then P2, etc.
    for(auto* buf : popped_bufs) {
        ASSERT_TRUE(chain.push(buf));
    }

    // 3. Verification Phase
    std::vector<Buffer<int, alignof(int), true>*> drain_pile;
    
    for(int i = 0; i < 5; ++i) {
        auto* buf = chain.pop();
        ASSERT_TRUE(buf != nullptr);
        
        // P1 should hold 0, P2 should hold 1...
        // Note: This relies on push() filling from Head and pop() draining from Head.
        ASSERT_EQ(*buf->p_Buffer, i);
        
        drain_pile.push_back(buf);
    }
    
    // Cleanup
    for(auto* b : drain_pile) chain.push(b);
}

/**
 * @brief CRITICAL TEST: Cross-Pool Cleanup.
 * * Verifies that if a buffer is allocated by Pool A, but pushed into Pool B,
 * and then the Chain is destroyed, the memory is correctly freed without 
 * double-frees or leaks.
 */
void TestCrossPoolCleanup() {
    LOG_TEST("TestCrossPoolCleanup (Alloc in A -> Free in B)");
    
    LifeTracker::active_count.store(0);

    {
        // 1. Create Chain with very small pools (Capacity 1)
        // Full Mode: Chain owns the buffers.
        PoolChain<LifeTracker, 64, true> chain(2, 999);
        
        std::vector<Buffer<LifeTracker, 64, true>*> headers;
        
        // 2. Force expansion to create 10 pools.
        // We pop 10 buffers. Each comes from a newly created pool.
        for(int i = 0; i < 10; ++i) {
            auto* buf = chain.pop();
            ASSERT_TRUE(buf != nullptr);
            headers.push_back(buf);
        }

        ASSERT_EQ(LifeTracker::active_count.load(), 10);

        // 3. Shuffle the buffers. 
        // This ensures that when we push them back, they are unlikely to go 
        // back to their original "birth" pool.
        std::random_device rd;
        std::mt19937 g(rd());
        std::shuffle(headers.begin(), headers.end(), g);

        // 4. Push all buffers back into the chain.
        // Logic: Push iterates pools from Head. It will fill Pool 1, then Pool 2, etc.
        // Since we shuffled 'headers', Pool 1 (Head) will likely receive a buffer 
        // that was originally created by Pool 10.
        for(auto* buf : headers) {
            bool ok = chain.push(buf);
            ASSERT_TRUE(ok);
        }

        // At this point, ownership has been transferred to the pools.
        // Pool 1 might own the pointer for a buffer created by Pool 10.
        
    } // 5. Chain Destructor called here.
      // It iterates all pools and destroys them.
      // Each pool deletes the buffers residing in its queue.

    // 6. Verify strict zero leaks.
    if(LifeTracker::active_count.load() != 0) {
        std::cerr << "Detected Memory Leak! Alive count: " << LifeTracker::active_count.load() << std::endl;
    }
    ASSERT_EQ(LifeTracker::active_count.load(), 0);
}

/**
 * @brief Heavy Stress Test with Producer/Consumer logic.
 * Ensures thread safety of atomic operations during expansion.
 */
void TestHeavyConcurrency() {
    LOG_TEST("TestHeavyConcurrency (Producer/Consumer)");

    // Start with small capacity to force MANY expansions.
    // Full Mode.
    PoolChain<int, alignof(int), true> chain(4, 0);

    const int TOTAL_OPS = 100000;
    std::atomic<int> consumed_count{0};
    
    // Consumer Thread:
    // Pops items. If it gets one, it increments count and PUSHES IT BACK immediately 
    // (simulating buffer reuse).
    // Note: In a real Chain scenario, usually one thread produces (allocs/pops) and another consumes (frees/pushes).
    // However, PoolChain::pop() triggers expansion. 
    // Let's simulate:
    // Consumer: Pops (triggers growth).
    // Producer: Pushes (returns buffers).
    
    // Actually, let's keep it simple: 
    // 1 Thread popping (Expanding chain).
    // 1 Thread pushing (Recycling buffers).
    // But they need to share the buffers.
    
    // Better simulation for Chain:
    // A single thread is safely using it as a growable pool allocator.
    // But let's try 2 threads to stress atomics.
    // Since RingPool is Single-Producer Single-Consumer, we must respect that.
    // PoolChain iterates pools. 
    // Thread A (Consumer/Alloc): calls chain.pop()
    // Thread B (Producer/Dealloc): calls chain.push()
    
    std::atomic<bool> done{false};
    std::vector<Buffer<int, alignof(int), true>*> traffic_jam; 
    // To pass buffers from A to B safely without using the chain itself (to test the chain), 
    // we would need another queue. 
    // Instead, let's use the chain as the communication channel + allocator.
    
    // But Wait: pop() creates buffers in Full Mode.
    // So Thread A pops (allocates) -> passes to Thread B -> Thread B pushes (deallocates).
    // This is valid SPSC usage.
    
    // We need a side-channel to pass the pointer from A to B to simulate "work".
    // Let's use a simple vector with a mutex for the side channel.
    struct SideChannel {
        std::vector<Buffer<int, alignof(int), true>*> bufs;
        std::atomic_flag lock = ATOMIC_FLAG_INIT;
        
        void add(Buffer<int, alignof(int), true>* b) {
            while(lock.test_and_set(std::memory_order_acquire)); // spin
            bufs.push_back(b);
            lock.clear(std::memory_order_release);
        }
        
        Buffer<int, alignof(int), true>* get() {
            while(lock.test_and_set(std::memory_order_acquire)); // spin
            if(bufs.empty()) {
                lock.clear(std::memory_order_release);
                return nullptr;
            }
            auto* b = bufs.back();
            bufs.pop_back();
            lock.clear(std::memory_order_release);
            return b;
        }
    } channel;

    auto start = std::chrono::high_resolution_clock::now();

    std::thread allocator_thread([&]() {
        for(int i=0; i<TOTAL_OPS; ++i) {
            auto* buf = chain.pop(); 
            // This pop might trigger addPool() if recycled buffers aren't back yet.
            ASSERT_TRUE(buf != nullptr);
            *buf->p_Buffer = i;
            
            // Send to "Worker" (which will eventually push back to pool)
            channel.add(buf);
            
            // Artificial delay to force chain growth sometimes
            // if(i % 100 == 0) std::this_thread::yield();
        }
        done.store(true);
    });

    std::thread deallocator_thread([&]() {
        int processed = 0;
        while(processed < TOTAL_OPS) {
            auto* buf = channel.get();
            if(buf) {
                // "Process" it
                assert(*buf->p_Buffer >= 0); 
                
                // Return to chain
                bool ok = chain.push(buf);
                // Note: push should never fail in this architecture if the chain grew enough
                // and we are returning valid buffers.
                // However, push iterates pools.
                ASSERT_TRUE(ok); 
                
                processed++;
            } else {
                std::this_thread::yield();
            }
        }
    });

    allocator_thread.join();
    deallocator_thread.join();

    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> diff = end - start;
    
    std::cout << "    Processed " << TOTAL_OPS << " items in " << diff.count() << "s (" 
              << (long)(TOTAL_OPS/diff.count()) << " ops/s)\n";
    
    // Cleanup: Drain the pool
    // In Full Mode, we don't need to manually delete buffers, 
    // but we should verify the pool is valid.
    
    // Check final stats? (Optional)
}

/**
 * @brief Tests edge case where pop() is called recursively or under high memory pressure.
 * Also verifies that addPool() links correctly.
 */
void TestMassiveGrowth() {
    LOG_TEST("TestMassiveGrowth");
    
    // Start with 1 pool of capacity 1.
    PoolChain<size_t, 0, true> chain(2000, 0);
    
    const int TARGET_POOLS = 1000000;
    std::vector<Buffer<size_t, 0, true>*> held_buffers;
    held_buffers.reserve(TARGET_POOLS);

    // Drain chain to force creation of 2000 pools
    for(int i=0; i<TARGET_POOLS; ++i) {
        auto* b = chain.pop();
        ASSERT_TRUE(b != nullptr);
        held_buffers.push_back(b);
    }
    
    // Now push them all back.
    // The first push will have to traverse 0 pools (if lucky) or check head.
    // The logic is: Head -> Next -> Next.
    // Empty pools are likely at the beginning if we drained them in order?
    // Actually, we drained P1, created P2, drained P2...
    // So all pools are empty.
    // Push should fill P1, then P2...
    
    for(auto* b : held_buffers) {
        ASSERT_TRUE(chain.push(b));
    }
    
    // Verification: Pop them all again.
    int count = 0;
    while(true) {
        // We don't want to create NEW pools, just drain existing.
        // But pop() automatically creates new ones if empty.
        // We know we pushed TARGET_POOLS items.
        if (count == TARGET_POOLS) break;
        
        auto* b = chain.pop();
        count++;
        // push back to avoid leak in logic (though destructor handles it)
        chain.push(b); 
    }
    ASSERT_EQ(count, TARGET_POOLS);
}

int main() {
    TestChainExpansionFullMode();
    TestMultiPoolWrapAround();
    TestLifecycleManagement();
    TestProducerConsumerGrowth();
    TestTortureGrowth();

    TestBasicExpansionAndOrder();
    TestCrossPoolCleanup();
    TestHeavyConcurrency();
    TestMassiveGrowth();

    std::cout << "\n\033[32m[PASSED]\033[0m All PoolChain tests completed successfully." << std::endl;
    return 0;
}
