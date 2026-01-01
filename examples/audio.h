#pragma once
#include <iostream>

#include "mem_sentry/heap.h"
#include "mem_sentry/sentry.h"

class Audio : public MEM_SENTRY::sentry::ISentry<Audio> {
    std::string member;// 32 bytes
    int integer; // 4 bytes 
    // 8 bytes added by VTP by the ISentry || 32 byte || 4 bytes || 4 bytes padding
    // overall 48

public:
    Audio() { 
        std::cout << "Audio Constructor" << "\n"; 
    }
    
    ~Audio(){
        std::cout << "Audio Destructor\n";
    }
};