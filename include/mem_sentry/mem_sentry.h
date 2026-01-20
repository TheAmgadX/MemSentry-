#pragma once
#include <new>
#include <cstddef> 

namespace MEM_SENTRY::heap {
    class Heap;
}

// --------------------------------------------------------------------------
// 1. Custom Operators
// --------------------------------------------------------------------------
void* operator new(size_t size, MEM_SENTRY::heap::Heap *pHeap);
void* operator new(size_t size, std::align_val_t alignment, MEM_SENTRY::heap::Heap* heap);

// --------------------------------------------------------------------------
// 2. Standard Replaceable Allocation Operators
// --------------------------------------------------------------------------

// Standard new/delete
void* operator new(size_t size);
void  operator delete(void* pMem) noexcept;

// Array new/delete
void* operator new[](size_t size);
void  operator delete[](void* pMem) noexcept;

// C++17 Aligned new/delete
void* operator new(size_t size, std::align_val_t al);
void* operator new[](size_t size, std::align_val_t al);
void  operator delete(void* pMem, std::align_val_t al) noexcept;
void  operator delete[](void* pMem, std::align_val_t al) noexcept;

// C++14 Sized delete (Optimization: helps if you know size during free)
void operator delete(void* ptr, std::size_t sz) noexcept;
void operator delete[](void* ptr, std::size_t sz) noexcept;
void operator delete(void* ptr, std::size_t sz, std::align_val_t al) noexcept;
void operator delete[](void* ptr, std::size_t sz, std::align_val_t al) noexcept;

// --------------------------------------------------------------------------
// 3. Nothrow Operators (Return nullptr instead of throwing)
// --------------------------------------------------------------------------
void* operator new(std::size_t count, const std::nothrow_t& tag) noexcept;
void* operator new[](std::size_t count, const std::nothrow_t& tag) noexcept;
void* operator new(std::size_t count, std::align_val_t al, const std::nothrow_t& tag) noexcept;
void* operator new[](std::size_t count, std::align_val_t al, const std::nothrow_t& tag) noexcept;

void operator delete(void* ptr, const std::nothrow_t& tag) noexcept;
void operator delete[](void* ptr, const std::nothrow_t& tag) noexcept;
void operator delete(void* ptr, std::align_val_t al, const std::nothrow_t& tag) noexcept;
void operator delete[](void* ptr, std::align_val_t al, const std::nothrow_t& tag) noexcept;