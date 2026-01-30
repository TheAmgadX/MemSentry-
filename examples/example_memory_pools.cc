// Example 5: Memory Pools Usage
// -------------------------------------------------------------
// This example demonstrates how to use Buffer and RingPool from mem_pools
// for efficient object reuse. Shows both dynamic (heap) and inline (stack)
// buffer usage, and how to use a RingPool for object pooling.
// -------------------------------------------------------------

#include "mem_pools/buffer.h"
#include "mem_pools/pool.h"
#include "mem_pools/chain.h"

#include "mem_sentry/heap.h"
#include "mem_sentry/sentry.h"
#include "mem_sentry/mem_sentry.h"

#include <iostream>
#include <vector>

// A simple struct to demonstrate pool usage
struct MyObj {
    int value;
    MyObj(int v) : value(v) { std::cout << "MyObj constructed: " << value << "\n"; }
    ~MyObj() { std::cout << "MyObj destructed: " << value << "\n"; }
};

int main() {
    // 1. Dynamic buffer (heap allocated, managed by Buffer)
    MEM_SENTRY::heap::Heap* heap = new MEM_SENTRY::heap::Heap("PoolHeap");
    MEM_SENTRY::mem_pool::Buffer<MyObj, 32, true>* dynBuf = new (heap) MEM_SENTRY::mem_pool::Buffer<MyObj, 32, true>(123);
    std::cout << "Dynamic buffer value: " << dynBuf->p_Buffer->value << "\n";

    // 2. Inline buffer (stack allocated, no heap allocation)
    MEM_SENTRY::mem_pool::Buffer<MyObj, 32, false> inlineBuf(456);
    std::cout << "Inline buffer value: " << inlineBuf.m_Buffer.value << "\n";

    // 3. RingPool usage (object pool for MyObj)
    MEM_SENTRY::mem_pool::RingPool<MyObj, 32, true> pool(false, 4, 789);
    
    if (auto* buf = pool.pop()) {
        std::cout << "Popped from pool: " << buf->p_Buffer->value << "\n";
        // Return buffer to pool for reuse
        pool.push(buf);
    }

    size_t COUNT = 100;


    // 4. Chain Pool Usage (Linked list of Ring Pools) can grow as needed.
    MEM_SENTRY::mem_pool::PoolChain<MyObj, 16, true> chainPool(4, -10); 
    std::vector<MEM_SENTRY::mem_pool::Buffer<MyObj, 16, true>*> buffers;
    buffers.reserve(COUNT);
    size_t counter = COUNT;

    // will grow as much as needed.
    while (counter) {
        auto* buf = chainPool.pop();
        std::cout << "Popped from pool: " << buf->p_Buffer->value << "\n";
        buffers.push_back(buf);
        counter--;
    }
    
    while(counter){
        // Return buffer to pool for reuse
        chainPool.push(buffers[counter - 1]);
        counter--;
    }

    buffers.clear();

    // Clean up dynamic buffer
    delete dynBuf;
    return 0;
}
