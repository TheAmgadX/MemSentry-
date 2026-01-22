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

int MEM_SENTRY::heap::Heap::CountAllocations() noexcept {
    std::lock_guard<std::mutex> lock(m_llMutex);

    int count = 0;
    alloc_header::AllocHeader* tmp = p_HeadList;
    
    while(tmp){
        ++count; 
        tmp = tmp->p_Next; 
    }
    
    return count;
}

void MEM_SENTRY::heap::Heap::AddAllocation(alloc_header::AllocHeader* alloc) {
    std::lock_guard<std::mutex> lock(m_llMutex);
    
    m_total += alloc->m_Size + alloc->m_Alignment;

    if (p_Reporter) {
        p_Reporter->onAlloc(alloc);
    }

    if(!addAllocLL(alloc)){
        std::printf("Error: error while manipulating Heap Allocations Linked List\n");
    }
}

void MEM_SENTRY::heap::Heap::RemoveAlloc(alloc_header::AllocHeader* alloc) {
    std::lock_guard<std::mutex> lock(m_llMutex);

    m_total -= alloc->m_Size + alloc->m_Alignment;

    if (p_Reporter) {
        p_Reporter->onDealloc(alloc);
    }

    if(!removeAllocLL(alloc)){
        std::printf("Error: error while manipulating Heap Allocations Linked List\n");
    }
}

void MEM_SENTRY::heap::Heap::ReportMemory(int bookMark1, int bookMark2){
    std::lock_guard<std::mutex> lock(m_llMutex);

    alloc_header::AllocHeader* tmp = p_HeadList;

    while(tmp && tmp->m_AllocId < bookMark1){
        tmp = tmp->p_Next;
    }
    
    int total = 0;

    while(tmp && tmp->m_AllocId <= bookMark2){
        total += tmp->m_Size;
        if (p_Reporter) {
            p_Reporter->report(tmp);
            printf("\n");
        }
        tmp = tmp->p_Next;
    }
}
