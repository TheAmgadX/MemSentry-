#include <assert.h>
#include <cstdint>

#include "mem_sentry/heap.h"
#include "mem_sentry/alloc_header.h"
#include "mem_sentry/constants.h"

// ============================================================================
// INTERNAL HELPER FUNCTIONS
// ============================================================================

/**
 * @brief Initializes the allocation header metadata.
 * Populates the header with tracking information, signatures, and the original
 * pointer required for deallocation.
 * 
 * @param size Bytes of user data requested.
 * @param alignment Alignment used.
 * @param originalAddr The raw pointer returned by malloc (crucial for free()).
 * @param pHeader Pointer to the location where the header resides.
 * @param pHeap The heap instance tracking this allocation.
 */
void set_alloc_header(size_t size, size_t alignment, char* originalAddr,
    MEM_SENTRY::alloc_header::AllocHeader* pHeader, MEM_SENTRY::heap::Heap *pHeap){

    pHeader->p_Heap = pHeap;
    pHeader->m_Size = size;
    pHeader->m_Alignment = alignment; 
    pHeader->m_Signature = MEM_SENTRY::constants::MEMSYSTEM_SIGNATURE;
    pHeader->m_AllocId = pHeap->GetNextId();
    pHeader->p_OriginalAddress = originalAddr;
}

/**
 * @brief Calculates a valid alignment size.
 * Ensures the requested alignment is a power of 2 and is at least as large
 * as a void pointer (to ensure our internal headers don't break alignment).
 * 
 * @param alignment The requested alignment enum.
 * 
 * @return size_t Validated alignment value.
 */
size_t calculate_aligned_memory_size(std::align_val_t alignment){
    size_t size = static_cast<size_t>(alignment);
    
    if(size < sizeof(void*)){
        size = sizeof(void*);
    }

    bool isPowerOf2 = size > 0 && (size & (size - 1)) == 0;

    assert(isPowerOf2 == true && "Alignment must be power of 2");

    return size;
}

// ============================================================================
// CORE ALLOCATION LOGIC
// ============================================================================

/**
 * @brief Allocates standard (unaligned/default aligned) memory.
 * Layout: [Header] [User Data] [Footer]
 * 
 * @param size Bytes requested by the user.
 * @param pHeap The heap to track this allocation.
 * 
 * @return void* Pointer to the start of the user data.
 */
void* sentry_allocate(size_t size, MEM_SENTRY::heap::Heap *pHeap){
    if(size == 0) 
        size = 1;
    
    size_t total_requested_memory = size + sizeof(MEM_SENTRY::alloc_header::AllocHeader) + sizeof(int);

    char *pMem = (char *)malloc(total_requested_memory);

    MEM_SENTRY::alloc_header::AllocHeader *pHeader = (MEM_SENTRY::alloc_header::AllocHeader *) pMem;
    
    set_alloc_header(size, 0, (char*)pHeader, pHeader, pHeap);
    
    pHeap->AddAllocation(pHeader);
    
    void *pStartBlock = pMem + sizeof(MEM_SENTRY::alloc_header::AllocHeader);
    
    int *pEndMarker = (int*) ((char*) pStartBlock + size);

    *pEndMarker = MEM_SENTRY::constants::MEMSYSTEM_ENDMARKER;

    return pStartBlock;
}

/**
 * @brief Allocates aligned memory.
 * Layout: [Padding?] [Header] [User Data (Aligned)] [Footer] [?Padding]
 * Uses pointer arithmetic to guarantee the user data starts on the requested boundary.
 * 
 * @param size Bytes requested by the user.
 * @param alignment Alignment requirement (must be power of 2).
 * @param pHeap The heap to track this allocation.
 * 
 * @return void* Pointer to the aligned user data.
 */
void* sentry_allocate_aligned(size_t size, size_t alignment, MEM_SENTRY::heap::Heap *pHeap){
    if(size == 0) 
        size = 1;

    uint16_t header_size = sizeof(MEM_SENTRY::alloc_header::AllocHeader);
    size_t total_requested_memory = size + alignment + header_size + sizeof(int); // int for the signature at the end of data.
    
    char *pOriginalMem = (char *) malloc(total_requested_memory);

    uintptr_t rawAddr = (uintptr_t) pOriginalMem;

    size_t data_size = size + sizeof(int); // data + signature.

    uintptr_t potential_data_start = rawAddr + header_size; 

    // the alignment must be power of 2, which is gauranteed via `calculate_aligned_memory_size()`
    size_t mask = alignment - 1;
    uintptr_t aligned_data_addr = (potential_data_start + mask) & ~mask;
    
    char* pMem = (char*) aligned_data_addr;

    // add end signature at the end.
    int* signature_addr = (int*)(pMem + size);
    *signature_addr = MEM_SENTRY::constants::MEMSYSTEM_ENDMARKER;

    // backward header_size bytes to get the header address.
    char* header_addr = (char*)(pMem - header_size); 
    MEM_SENTRY::alloc_header::AllocHeader *pHeader = (MEM_SENTRY::alloc_header::AllocHeader *) header_addr;

    set_alloc_header(size, alignment, pOriginalMem, pHeader, pHeap);

    pHeap->AddAllocation(pHeader);

    return pMem;
}

/**
 * @brief Unified deallocation function.
 * Works for both standard and aligned allocations because it retrieves
 * the 'p_OriginalAddress' from the header.
 * 
 * @param pMem Pointer to the user data to free.
 */
void sentry_deallocate(void *pMem){
    if (!pMem) return;
    
    // Backtrack to find the header
    MEM_SENTRY::alloc_header::AllocHeader *pHeader = (MEM_SENTRY::alloc_header::AllocHeader *) (
        (char *)pMem - sizeof(MEM_SENTRY::alloc_header::AllocHeader)
    );

    // to make sure we don't free data that is not allocated by our memory manager.
    assert(pHeader->m_Signature == MEM_SENTRY::constants::MEMSYSTEM_SIGNATURE);

    // mark as freed memory.
    pHeader->m_Signature = MEM_SENTRY::constants::MEMSYSTEM_FREED_SIGNATURE;

    int* pEndMarker = (int*) ((char *)pMem + pHeader->m_Size);

    /*
        make sure the end marker is with our signature to avoid free beyond the array.
    */ 
    assert(*pEndMarker == MEM_SENTRY::constants::MEMSYSTEM_ENDMARKER); 

    pHeader->p_Heap->RemoveAlloc(pHeader);

    free(pHeader->p_OriginalAddress);
}

// ============================================================================
// GLOBAL OPERATOR OVERRIDES
// ============================================================================

// --- Standard Scalar ---
void* operator new(size_t size, MEM_SENTRY::heap::Heap *pHeap) {
#if MEM_SENTRY_ENABLE
    return sentry_allocate(size, pHeap);
#else
    return malloc(size);
#endif
}

void* operator new(size_t size) {
    return ::operator new(size, MEM_SENTRY::heap::HeapFactory::GetDefaultHeap());
}    

void operator delete (void *pMem) {
#if MEM_SENTRY_ENABLE
    sentry_deallocate(pMem);
#else
    free(pMem);
#endif
}

// --- Standard Array ---
void* operator new[](size_t size){
    return ::operator new(size, MEM_SENTRY::heap::HeapFactory::GetDefaultHeap());
}

void operator delete[](void* pMem){
    ::operator delete(pMem);
}

// --- Aligned Scalar ---
void* operator new(size_t size, std::align_val_t alignment, MEM_SENTRY::heap::Heap *pHeap) {
    size_t alignment_size = calculate_aligned_memory_size(alignment);
#if MEM_SENTRY_ENABLE
    return sentry_allocate_aligned(size, alignment_size, pHeap);
#else
    // normal aligned allocation.
    return std::aligned_alloc(size, alignment_size);
#endif
}

void* operator new(size_t size, std::align_val_t alignment){
    return ::operator new(size, alignment, MEM_SENTRY::heap::HeapFactory::GetDefaultHeap());
}

// --- Aligned Array ---
void* operator new[](size_t size, std::align_val_t alignment) {
    return ::operator new(size, alignment);
}