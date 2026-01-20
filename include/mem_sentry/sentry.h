#pragma once

#include "mem_sentry/heap.h"
#include "mem_sentry/mem_sentry.h"

namespace MEM_SENTRY::sentry {

    /**
     * @class ISentry
     * @brief A CRTP base class that automates memory tracking for derived classes.
     * 
     * Inherit from this class to force all dynamic allocations (via `new`) of your object
     * to be routed through the MEM_SENTRY memory system. It automatically manages 
     * a static heap pointer unique to each derived class type.
     * 
     * @tparam T The derived class type (Curiously Recurring Template Pattern).
     * Example: class MyClass : public ISentry<MyClass> {};
     */
    template<typename T>
    class ISentry { 
    private:
        /**
         * @brief Lazy initialization of the internal heap pointer.
         * 
         * Ensures that if the user hasn't manually called setHeap(), 
         * we default to the global default heap rather than crashing on a null pointer.
         */
        static void checkHeap() noexcept {
            pHeap = !pHeap ? MEM_SENTRY::heap::HeapFactory::GetDefaultHeap() : pHeap;
        }

    public:
        /// @brief The specific heap instance used for allocating objects of type T.
        /// Unique for every class T due to the template nature of ISentry.
        static MEM_SENTRY::heap::Heap* pHeap;
        
        /**
         * @brief Virtual destructor.
         * @note Adds a virtual table pointer (vptr) to the object
         * 
         * Required to ensure derived destructors are called correctly when deleting via ISentry*.
         */
        virtual ~ISentry() = default; 

        /**
         * @brief Assigns a specific memory heap to this class type.
         * 
         * After calling this, all `new T` calls will allocate from the provided heap.
         * 
         * @param heap Pointer to the target heap.
         */
        static void setHeap(MEM_SENTRY::heap::Heap* heap){
            pHeap = heap;
        }

        // ========================================================================
        // STANDARD ALLOCATION
        // Routes standard `new T` and `new T[]` to the tracked heap.
        // ========================================================================

        /**
         * @brief Standard operator new override.
         * Allocates memory from the class's assigned heap (pHeap).
         */
        void* operator new(size_t size){
            checkHeap();
            return ::operator new(size, pHeap);
        }

        /**
         * @brief Standard array operator new[] override.
         * Allocates arrays from the class's assigned heap (pHeap).
         */
        void* operator new[](size_t size){
            checkHeap();
            return ::operator new(size, pHeap);
        }

        // ========================================================================
        // ALIGNED ALLOCATION
        // Handles `new T` where T has alignment requirements.
        // ========================================================================

        /**
         * @brief Aligned operator new override.
         * Used automatically for types with `alignas(X)` specifiers.
         */
        void* operator new(size_t size, std::align_val_t alignment){
            checkHeap();
            return ::operator new(size, alignment, pHeap);
        }

        /**
         * @brief Aligned array operator new[] override.
         */
        void* operator new[](size_t size, std::align_val_t alignment){
            checkHeap();
            return ::operator new(size, alignment, pHeap);
        }

        // ========================================================================
        // NOTHROW ALLOCATION
        // Handles `new (std::nothrow) T`. Returns nullptr on failure instead of throwing.
        // ========================================================================

        void* operator new(size_t size, const std::nothrow_t& tag) noexcept {
            checkHeap();
            try { return ::operator new(size, pHeap); }
            catch(...) { return nullptr; }
        }

        void* operator new[](size_t size, const std::nothrow_t& tag) noexcept {
            checkHeap();
            try { return ::operator new(size, pHeap); }
            catch(...) { return nullptr; }
        }

        void* operator new(size_t size, std::align_val_t alignment, const std::nothrow_t& tag) noexcept {
            checkHeap();
            try { return ::operator new(size, alignment, pHeap); }
            catch(...) { return nullptr; }
        }

        void* operator new[](size_t size, std::align_val_t alignment, const std::nothrow_t& tag) noexcept {
            checkHeap();
            try { return ::operator new(size, alignment, pHeap); }
            catch(...) { return nullptr; }
        }

        // ========================================================================
        // RESTORATION (PASS-THROUGHS)
        // These operators are normally hidden when a class defines its own `new`.
        // We explicitly restore them to ensure standard C++ behavior works as expected.
        // ========================================================================

        /**
         * @brief Placement New Pass-through.
         * Allows constructing objects in pre-allocated buffers (e.g., `new (buffer) T()`).
         * 
         * @note This does NOT allocate memory and is NOT tracked by MemSentry.
         */
        void* operator new(size_t, void* ptr) noexcept { return ptr; }
        
        /**
         * @brief Placement Array New Pass-through.
         */
        void* operator new[](size_t, void* ptr) noexcept { return ptr; }

        /**
         * @brief Explicit Heap Override.
         * Allows bypassing the class's static heap for a specific allocation.
         * Usage: `new (mySpecificHeap) T()`
         */
        void* operator new(size_t size, MEM_SENTRY::heap::Heap* h) {
            return ::operator new(size, h);
        }

        /**
         * @brief Explicit Heap Override (Aligned).
         */
        void* operator new(size_t size, std::align_val_t al, MEM_SENTRY::heap::Heap* h) {
            return ::operator new(size, al, h);
        }
    };

    // Static member initialization
    template<typename T>
    MEM_SENTRY::heap::Heap* ISentry<T>::pHeap = nullptr;
};