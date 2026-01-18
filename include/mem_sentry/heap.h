#pragma once
#include <iostream>
#include <cstring>


namespace MEM_SENTRY::heap {       
    class Heap {
        char m_name[100];
        int m_total;
        int m_NextAllocId;
        alloc_header::AllocHeader* p_HeadList;
        alloc_header::AllocHeader* p_TailList;

        void printAlloc(alloc_header::AllocHeader* alloc);
    public:
        Heap(const char *name) {
            std::strncpy(m_name, name, 99);
            m_name[99] = '\0';
            m_total = 0;

            p_HeadList = nullptr;
            p_TailList = nullptr;
        }
        
        const char * GetName() const { return m_name; }
        
        void AddAllocation(int size);
    
        void RemoveAlloc(int size);
    
        int GetMemoryBookMark() const noexcept;

        void ReportMemory(int bookMark1, int bookMark2);
    };
    
    
    class HeapFactory {
    public:
        static Heap* GetDefaultHeap() {
            static Heap defaultHeap("DefaultHeap");
            return &defaultHeap;
        }
    };
};

