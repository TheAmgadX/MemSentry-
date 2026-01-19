#include <iostream>

#include "mem_sentry/heap.h"
#include "mem_sentry/alloc_header.h"

bool MEM_SENTRY::heap::Heap::addAllocLL(alloc_header::AllocHeader* alloc){
    if(!alloc) 
        return false;
    
    // if allocations list is empty.
    if(!p_HeadList){
        p_HeadList = alloc;
        p_TailList = alloc;

        // initialize links for single node
        alloc->p_Next = nullptr;
        alloc->p_Prev = nullptr;

        return true;
    }
    
    // add to the end and update tail.
    p_TailList->p_Next = alloc;
    alloc->p_Prev = p_TailList;
    alloc->p_Next = nullptr;
    p_TailList = alloc;

    return true;    
}

bool MEM_SENTRY::heap::Heap::removeAllocLL(alloc_header::AllocHeader* alloc){
    if(!alloc)
        return false;
    
    // NOTE: this function won't `delete alloc` because this is handled in the overriden delete.

    // if allocations list is empty.
    if(!p_HeadList){
        return false;
    }
    
    // only one node
    if(p_HeadList == p_TailList){
        p_HeadList = nullptr;
        p_TailList = nullptr;
        return true;
    }

    // if the alloc is the head.
    if(alloc == p_HeadList){
        p_HeadList = alloc->p_Next;
        
        // always will be valid.
        p_HeadList->p_Prev = nullptr;
        
        return true;
    }

    // if the alloc is the tail
    if(alloc == p_TailList){
        p_TailList = alloc->p_Prev;
        if(p_TailList) p_TailList->p_Next = nullptr;
        return true;
    }
    
    // if alloc is in the middle, connect nodes.
    alloc_header::AllocHeader* next = alloc->p_Next;
    alloc_header::AllocHeader* prev = alloc->p_Prev;
    
    prev->p_Next = next;
    next->p_Prev = prev;

    return true;
}

int MEM_SENTRY::heap::Heap::CountAllocations() const noexcept {
    int count = 0;
    alloc_header::AllocHeader* tmp = p_HeadList;
    
    while(tmp){
        ++count; 
        tmp = tmp->p_Next; 
    }
    
    return count;
}

void MEM_SENTRY::heap::Heap::AddAllocation(alloc_header::AllocHeader* alloc) {
    int size = alloc->m_Size;
    std::cout << "-----------------\n";
    std::cout << "Allocating " << size << " bytes on heap: " << m_name << "\n";
    m_total += size;
    std::cout << "Current total size is: " << m_total << " bytes on heap: " << m_name << "\n";
    std::cout << "-----------------\n";

    if(!addAllocLL(alloc)){
        std::printf("Error: error while manipulating Heap Allocations Linked List\n");
    }
}

void MEM_SENTRY::heap::Heap::RemoveAlloc(alloc_header::AllocHeader* alloc) {
    int size = alloc->m_Size;
    std::cout << "-----------------\n";
    std::cout << "Freeing " << size << " bytes from heap: " << m_name << "\n";
    m_total -= size;
    std::cout << "Current total size is: " << m_total << " bytes on heap: " << m_name << "\n";
    std::cout << "-----------------\n";

    if(!removeAllocLL(alloc)){
        std::printf("Error: error while manipulating Heap Allocations Linked List\n");
    }
}

void MEM_SENTRY::heap::Heap::printAlloc(alloc_header::AllocHeader* p_Alloc, int totalMemoryToThisPoint){
    std::printf("---------------------------------\n");
    std::printf("Heap Name:     %s\n",     p_Alloc->p_Heap->m_name);
    std::printf("Allocation Id: %d\n",     p_Alloc->m_AllocId);
    std::printf("Signature:     0x%X\n",   p_Alloc->m_Signature);
    std::printf("Size:          %d bytes\n", p_Alloc->m_Size);
    std::printf("Total Memory:  %d bytes\n", totalMemoryToThisPoint);
    std::printf("---------------------------------\n");
}

void MEM_SENTRY::heap::Heap::ReportMemory(int bookMark1, int bookMark2){
    alloc_header::AllocHeader* tmp = p_HeadList;

    while(tmp && tmp->m_AllocId < bookMark1){
        tmp = tmp->p_Next;
    }
    
    int total = 0;

    while(tmp && tmp->m_AllocId <= bookMark2){
        total += tmp->m_Size;
        printAlloc(tmp, total);
        printf("\n");
        tmp = tmp->p_Next;
    }
}
