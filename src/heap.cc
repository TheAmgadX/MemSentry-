#include <iostream>

#include "mem_sentry/heap.h"
#include "mem_sentry/alloc_header.h"


void MEM_SENTRY::heap::Heap::AddAllocation(int size) {
    std::cout << "-----------------\n";
    std::cout << "Allocating " << size << " bytes on heap: " << m_name << "\n";
    m_total += size;
    std::cout << "Current total size is: " << m_total << " bytes on heap: " << m_name << "\n";
    std::cout << "-----------------\n";
}

void MEM_SENTRY::heap::Heap::RemoveAlloc(int size) {
    std::cout << "-----------------\n";
    std::cout << "Freeing " << size << " bytes from heap: " << m_name << "\n";
    m_total -= size;
    std::cout << "Current total size is: " << m_total << " bytes on heap: " << m_name << "\n";
    std::cout << "-----------------\n";
}

int MEM_SENTRY::heap::Heap::GetMemoryBookMark() const noexcept {
    return m_NextAllocId;
}

void MEM_SENTRY::heap::Heap::printAlloc(alloc_header::AllocHeader* p_Alloc){
    std::printf("---------------------------------\n");
    std::printf("Heap Name:     %s\n",     p_Alloc->p_Heap->m_name);
    std::printf("Allocation Id: %d\n",     p_Alloc->m_AllocId);
    std::printf("Signature:     0x%X\n",   p_Alloc->m_Signature);
    std::printf("Size:          %d bytes\n", p_Alloc->m_Size);
    std::printf("---------------------------------\n");
}

void MEM_SENTRY::heap::Heap::ReportMemory(int bookMark1, int bookMark2){
    alloc_header::AllocHeader* tmp = p_HeadList;

    while(tmp && tmp->m_AllocId != bookMark1){
        tmp = tmp->p_Next;
    }

    while(tmp && tmp->m_AllocId != bookMark2){
        printAlloc(tmp);
        printf("\n");
        tmp = tmp->p_Next;
    }
}