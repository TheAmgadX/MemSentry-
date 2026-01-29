#pragma once
#include "mem_pools/concepts.h"
#include "mem_pools/pool.h"
#include "mem_pools/buffer.h"

#include <functional>

namespace MEM_SENTRY::mem_pool {

template<MEM_SENTRY::concepts::NotRawArray T, size_t alignment = 0, bool isDynamic = true>
struct ChainNode {
    /**
     * @brief Pointer to the RingPool owned by this node.
     *
     * Stored in a cache-line aligned atomic to permit lock-free
     * traversal/inspection from other threads. 
     * 
     * The atomic stores a raw pointer to the `RingPool`
     * instance that this node represents.
     */
    CacheAlignedAtomic<RingPool<T, alignment, isDynamic>*> m_Pool;

    /**
     * @brief Atomic pointer to the next ChainNode in the linked list.
     *
     * Used to traverse the chain from head -> tail. Stored in a
     * cache-line aligned atomic to avoid false-sharing.
     */
    CacheAlignedAtomic<ChainNode*> m_Next;

    /**
     * @brief Construct a ChainNode wrapping the given pool.
     *
     * The constructor stores the provided `pool` pointer into the
     * `m_Pool` atomic and initializes `m_Next` to `nullptr`.
     *
     * @param pool Pointer to a `RingPool` instance owned by the chain.
     */
    ChainNode(RingPool<T, alignment, isDynamic>* pool){
        m_Pool.m_Value.store(pool, std::memory_order_relaxed);
        m_Next.m_Value.store(nullptr, std::memory_order_relaxed);
    }
};

/**
 * @brief Lock-free chain of ring pools (growable pool-of-pools).
 *
 * The PoolChain provides a linked list of `RingPool` instances. Each
 * `ChainNode` holds a pointer to one `RingPool` and a pointer to the next node.
 * 
 * The chain starts with a single pool and may grow by appending new
 * pools when `pop()` is called and no buffers are available in existing
 * pools.
 *
 * Design notes:
 * - Each `RingPool` is a single-producer / single-consumer SPSC queue of
 *   `Buffer<T>` pointers. `PoolChain` iterates pools to `push()` or `pop()`.
 * 
 * - `PoolChain` performs lock-free traversal using atomics. It is intended
 *   for the single-writer/single-reader producer-consumer patterns that the
 *   underlying `RingPool` expects: typically one thread calls `pop()` to
 *   acquire buffers and another thread calls `push()` to return them.
 * 
 * - `addPool()` appends a new pool at the tail. The chain owns the
 *   `RingPool` instances and deletes them in the destructor.
 * 
 * - The constructor accepts a `queue_size` and forwards any additional
 *   variadic arguments to the underlying `RingPool` constructor.
 *
 * Thread safety:
 * - `push()` and `pop()` iterate the chain reading atomically-updated
 *   pointers. `addPool()` appends to the tail and updates `m_Tail`.
 * 
 * - The implementation assumes a single writer for `addPool()`/tail updates.
 * 
 * @note the cleanup is not thread safe and it's called in the destructor of the `PoolChain`.
 */
template<MEM_SENTRY::concepts::NotRawArray T, size_t alignment = 0, bool isDynamic = true>
class PoolChain {
private:
    /**
     * @brief Head node of the chain (first pool).
     *
     * Atomic pointer so readers iterating the chain observe a
     * consistent head without locks.
     */
    CacheAlignedAtomic<ChainNode<T, alignment, isDynamic>*> m_Head;

    /**
     * @brief Tail node of the chain (last pool).
     *
     * Used when appending a new pool via `addPool()`.
     */
    CacheAlignedAtomic<ChainNode<T, alignment, isDynamic>*> m_Tail;

    /**
     * @brief Factory lambda to create new `RingPool` instances.
     *
     * The factory captures the queue size and any forwarded
     * constructor arguments and returns an allocated `RingPool*`.
     */
    std::function<RingPool<T, alignment, isDynamic>*()> m_PoolFactory;   
private:

    /**
     * @brief Round up to next power of 2
     */
    static constexpr size_t next_power_of_2(size_t n) {
        if (n <= 1) return 1;
        n--;
        n |= n >> 1;
        n |= n >> 2;
        n |= n >> 4;
        n |= n >> 8;
        n |= n >> 16;
        n |= n >> 32;
        return n + 1;
    }
    
    /**
     * @brief Append a new `RingPool` instance to the tail of the chain.
     *
     * This allocates a `RingPool` via `m_PoolFactory` and links a new
     * `ChainNode` at the end of the list. 
     * 
     * The operation uses atomic stores to publish the new node.
     */
    void addPool();

    /**
     * @brief Destroy all pools and chain nodes owned by this `PoolChain`.
     *
     * Iterates the linked list deleting each `RingPool` instance and
     * then deleting nodes. 
     * 
     * @note Not thread-safe
     * 
     * intended to be called from the destructor when no other threads access the chain.
     */
    void cleanup();

public:

    /**
     * @brief Construct a `PoolChain` with an initial pool.
     *
     * The constructor rounds `queue_size` up to the next power-of-two,
     * constructs a factory lambda that creates `RingPool` instances and
     * then immediately allocates the first pool/node placed at head/tail.
     *
     * @tparam Args Variadic template parameters forwarded to `RingPool`.
     * @param queue_size Requested per-pool queue size (rounded up).
     * @param args Arguments forwarded to the `RingPool` constructor.
     */
    template<typename... Args>
    PoolChain(size_t queue_size, Args&&... args){
        queue_size = next_power_of_2(queue_size);

        m_PoolFactory = [=, this](){
            return new RingPool<T, alignment, isDynamic>(false, queue_size, args...);
        };

        RingPool<T, alignment, isDynamic>* pool = m_PoolFactory();
        
        ChainNode<T, alignment, isDynamic>* node = new ChainNode(pool);

        m_Head.m_Value.store(node, std::memory_order_relaxed);
        m_Tail.m_Value.store(node, std::memory_order_relaxed);
    }

    /**
     * @brief Destroy the chain and free owned `RingPool` instances.
     *
     * Calls `cleanup()` which deletes all pools and nodes. This
     * destructor assumes exclusive access to the chain.
     */
    ~PoolChain(){
        cleanup();
    }

    /**
     * @brief Attempt to push a buffer into the first pool with free space.
     *
     * Iterates the chain from head to tail and calls `push()` on each
     * `RingPool` until one accepts the buffer.
     *
     * @param buffer Pointer to the buffer to return to the chain.
     * 
     * @return `true` if pushed successfully, `false` otherwise.
     */
    bool push(Buffer<T, alignment, isDynamic>* buffer);

    /**
     * @brief Pop a buffer from the first pool that has available data.
     *
     * Iterates pools from head to tail calling `pop()` on each. If no
     * buffers are available in any existing pool, this function calls
     * `addPool()` to append a new pool and then pops from it.
     *
     * @return Pointer to a `Buffer` on success, or `nullptr` if the
     * last created pool failed to provide one.
     */
    Buffer<T, alignment, isDynamic>* pop();
};
}

template<MEM_SENTRY::concepts::NotRawArray T, size_t alignment, bool isDynamic>
void MEM_SENTRY::mem_pool::PoolChain<T, alignment, isDynamic>::addPool(){
    RingPool<T, alignment, isDynamic> *pool = m_PoolFactory();
    ChainNode<T, alignment, isDynamic>* node = new ChainNode<T, alignment, isDynamic>(pool);
    
    ChainNode<T, alignment, isDynamic>* current_tail = m_Tail.m_Value.load(std::memory_order_acquire);

    current_tail->m_Next.m_Value.store(node, std::memory_order_release);

    // Single Writer so relaxed here is safe.
    m_Tail.m_Value.store(node, std::memory_order_relaxed);
}

template<MEM_SENTRY::concepts::NotRawArray T, size_t alignment, bool isDynamic>
void MEM_SENTRY::mem_pool::PoolChain<T, alignment, isDynamic>::cleanup(){
    ChainNode<T, alignment, isDynamic>* current = m_Head.m_Value.load(std::memory_order_relaxed);

    while(current){
        ChainNode<T, alignment, isDynamic>* next = current->m_Next.m_Value.load(std::memory_order_relaxed);
        
        RingPool<T, alignment, isDynamic>* pool = current->m_Pool.m_Value.load(std::memory_order_relaxed);

        if(pool){
            delete pool;
        }

        delete current;

        current = next;
    }

    m_Head.m_Value.store(nullptr, std::memory_order_relaxed);
    m_Tail.m_Value.store(nullptr, std::memory_order_relaxed);
}

template<MEM_SENTRY::concepts::NotRawArray T, size_t alignment, bool isDynamic>
bool MEM_SENTRY::mem_pool::PoolChain<T, alignment, isDynamic>::push(Buffer<T, alignment, isDynamic>* buffer){
    ChainNode<T, alignment, isDynamic>* current = m_Head.m_Value.load(std::memory_order_acquire);

    while(current){
        ChainNode<T, alignment, isDynamic>* next = current->m_Next.m_Value.load(std::memory_order_acquire);
        
        RingPool<T, alignment, isDynamic>* pool = current->m_Pool.m_Value.load(std::memory_order_acquire);

        if(pool->push(buffer)){
            return true;
        }

        current = next;
    }

    return false;
}

template<MEM_SENTRY::concepts::NotRawArray T, size_t alignment, bool isDynamic>
MEM_SENTRY::mem_pool::Buffer<T, alignment, isDynamic>* MEM_SENTRY::mem_pool::PoolChain<T, alignment, isDynamic>::pop(){
    ChainNode<T, alignment, isDynamic>* current = m_Head.m_Value.load(std::memory_order_acquire);

    while(current){
        ChainNode<T, alignment, isDynamic>* next = current->m_Next.m_Value.load(std::memory_order_acquire);
        
        RingPool<T, alignment, isDynamic>* pool = current->m_Pool.m_Value.load(std::memory_order_acquire);

        Buffer<T, alignment, isDynamic>* buffer = pool->pop();

        if(buffer){
            return buffer;
        }

        current = next;
    }

    addPool();

    current = m_Tail.m_Value.load(std::memory_order_acquire);
    RingPool<T, alignment, isDynamic>* last_pool = current->m_Pool.m_Value.load(std::memory_order_acquire);

    return last_pool->pop(); 
}