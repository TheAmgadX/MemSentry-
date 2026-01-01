#pragma once

#include "mem_sentry/heap.h"
#include "mem_sentry/mem_sentry.h"

namespace MEM_SENTRY::sentry {
    /**
     * CRTP (Curiously Recurring Template Pattern) Wrapper.
     * NOTE: We use a template <T> here to ensure that every class (Effect, Audio, etc.)
     * gets its own UNIQUE static 'pHeap' variable. 
     * Without this template, all classes would share the same single pointer.
     */
    template<typename T>
    class ISentry { 
    public:
        static MEM_SENTRY::heap::Heap* pHeap;
        
        /**
         * NOTE: will add 8 bytes to the tracker due to the virtual table pointer.
         */
        virtual ~ISentry() = default; 

        /// @brief assign the heap of the class.
        /// @param heap which will be used to track this class.
        static void setHeap(MEM_SENTRY::heap::Heap* heap){
            pHeap = heap;
        }

        /**
         * @brief Overload new to use the static heap pointer
         * @param size number of bytes requested to be allocated. 
         * @return pointer to the requested memory
         */
        void* operator new(size_t size){
            pHeap = !pHeap ? MEM_SENTRY::heap::HeapFactory::GetDefaultHeap() : pHeap;
            return ::operator new(size, pHeap);
        }
    };

    // initialize the pHeap.
    template<typename T>
    MEM_SENTRY::heap::Heap* ISentry<T>::pHeap = nullptr;
};

