#pragma once
#include <iostream>
#include "mem_sentry/heap.h"

void* operator new(size_t size, MEM_SENTRY::heap::Heap *pHeap);
void  operator delete (void *pMem);
void* operator new(size_t size);
