#pragma once
#include <iostream>

#include "mem_sentry/heap.h"
#include "mem_sentry/sentry.h"

class Effect : public MEM_SENTRY::sentry::ISentry<Effect> {
    std::string member;// 32 bytes
    int64_t integer;// 8 bytes
    char* chars[9]; // 9 * 8 bytes
    char c; // 1 byte
    char * arr; // 8 bytes
    // 8 bytes added by VTP by the ISentry || 32 byte || 8 byte || 72 byte || 1 byte || 7 bytes padding || 8 bytes
    // overall 136

public:
    Effect() { 
        std::cout << "Effect Constructor" << "\n"; 
        arr = new char[8]; // 8 bytes in Default heap.
    }
    
    ~Effect(){
        std::cout << "Effect Destructor\n";
        delete[] arr; 
    }
};