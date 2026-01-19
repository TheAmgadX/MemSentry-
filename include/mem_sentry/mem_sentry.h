#pragma once
#include <iostream>
#include "mem_sentry/heap.h"

void* operator new(size_t size, MEM_SENTRY::heap::Heap *pHeap);
void* operator new(size_t size, std::align_val_t alignment, MEM_SENTRY::heap::Heap* heap);

void* operator new[](size_t size);
void* operator new(size_t size, std::align_val_t al);
void* operator new[](size_t size, std::align_val_t al);

void  operator delete[] (void *pMem);
void  operator delete (void *pMem, std::align_val_t al);
void  operator delete[] (void *pMem, std::align_val_t al);

void* operator new(size_t size);
void  operator delete (void* pMem);

