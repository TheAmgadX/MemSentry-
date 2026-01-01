#pragma once
#include <iostream>

#include "mem_sentry/heap.h"

class Effect {
    static MEM_SENTRY::heap::Heap* pEffectHeap;

    std::string member;// 32 bytes
    int64_t integer;// 8 bytes
    char* chars[9]; // 9 * 8 bytes
    char c; // 1 byte
    char * arr; // 8 bytes
    // 32 byte || 8 byte || 72 byte || 1 byte || 7 bytes padding || 8 bytes
    // overall 128

public:
    Effect() { 
        std::cout << "Effect Constructor" << "\n"; 
        arr = new char[8]; // 8 bytes in Default heap.
    }
    
    ~Effect(){
        std::cout << "Effect Destructor\n";
        delete[] arr; 
    }

    // Setter to configure the heap before use
    static void SetHeap(MEM_SENTRY::heap::Heap* pHeap) {
        pEffectHeap = pHeap;
    }

    // Overload new to use the static heap pointer
    void* operator new(size_t size) {
        // Accessing static member is valid here
        return ::operator new(size, pEffectHeap);
    }
    
    void operator delete(void* pMem) {
        ::operator delete(pMem);
    }
};

// Initialize static member
MEM_SENTRY::heap::Heap* Effect::pEffectHeap = nullptr;
