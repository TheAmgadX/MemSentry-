#pragma once
#include <iostream>
#include <cstring>


namespace MEM_SENTRY::heap {    
    class Heap {
        char m_name[100];
        int m_total;
    public:
        Heap(const char *name) {
            std::strncpy(m_name, name, 99);
            m_name[99] = '\0';
            m_total = 0;
        }
        
        const char * GetName() const { return m_name; }
        
        void AddAllocation(int size) {
            std::cout << "-----------------\n";
            std::cout << "Allocating " << size << " bytes on heap: " << m_name << "\n";
            m_total += size;
            std::cout << "Current total size is: " << m_total << " bytes on heap: " << m_name << "\n";
            std::cout << "-----------------\n";
        }
    
        void RemoveAlloc(int size) {
            std::cout << "-----------------\n";
            std::cout << "Freeing " << size << " bytes from heap: " << m_name << "\n";
            m_total -= size;
            std::cout << "Current total size is: " << m_total << " bytes on heap: " << m_name << "\n";
            std::cout << "-----------------\n";
        }
    };
    
    
    class HeapFactory {
    public:
        static Heap* GetDefaultHeap() {
            static Heap defaultHeap("DefaultHeap");
            return &defaultHeap;
        }
    };
};

