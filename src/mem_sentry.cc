#include <assert.h>

#include "mem_sentry/heap.h"
#include "mem_sentry/alloc_header.h"
#include "mem_sentry/constants.h"

//? _________________________ GLOBAL new/delete OPERATORS _________________________

void* operator new(size_t size, MEM_SENTRY::heap::Heap *pHeap) {
    size_t iRequestedBytes = size + sizeof(MEM_SENTRY::alloc_header::AllocHeader) + sizeof(int);

    char *pMem = (char *)malloc(iRequestedBytes);

    MEM_SENTRY::alloc_header::AllocHeader *pHeader = (MEM_SENTRY::alloc_header::AllocHeader *) pMem;

    pHeader->pHeap = pHeap;
    pHeader->iSize = size;
    pHeader->iSignature = MEM_SENTRY::constants::MEMSYSTEM_SIGNATURE;

    pHeap->AddAllocation(size);

    void *pStartBlock = pMem + sizeof(MEM_SENTRY::alloc_header::AllocHeader);
    
    int *pEndMarker = (int*) ((char*) pStartBlock + size);

    *pEndMarker = MEM_SENTRY::constants::MEMSYSTEM_ENDMARKER;

    return pStartBlock;
}

void operator delete (void *pMem) {
    if (!pMem) return;
    
    // Backtrack to find the header
    MEM_SENTRY::alloc_header::AllocHeader *pHeader = (MEM_SENTRY::alloc_header::AllocHeader *) (
        (char *)pMem - sizeof(MEM_SENTRY::alloc_header::AllocHeader)
    );

    // to make sure we don't free data that is not allocated by our memory manager.
    assert(pHeader->iSignature == MEM_SENTRY::constants::MEMSYSTEM_SIGNATURE);

    int* pEndMarker = (int*) ((char *)pMem + pHeader->iSize);

    /*
        make sure the end marker is with our signature to avoid free beyond the array.
    */ 
    assert(*pEndMarker == MEM_SENTRY::constants::MEMSYSTEM_ENDMARKER); 

    pHeader->pHeap->RemoveAlloc(pHeader->iSize);

    free(pHeader);
}

// Standard new implementation using default heap
void* operator new(size_t size) {
    return operator new(size, MEM_SENTRY::heap::HeapFactory::GetDefaultHeap());
}
