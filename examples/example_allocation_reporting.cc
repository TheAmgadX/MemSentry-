// Example 1: Allocation/Deallocation Reporting
// -------------------------------------------------------------
// This example demonstrates how to use MemSentry to track and report
// memory allocations and deallocations. It shows how to attach a
// ConsoleReporter to a Heap, and how memory events are reported.
//
// Key points:
//  - Use `Heap` to manage a memory category.
//  - Attach a `ConsoleReporter` to log allocation/deallocation events.
//  - Use placement new with the heap to allocate objects.
//  - All allocations and frees are reported to the reporter.
// -------------------------------------------------------------

#include "mem_sentry/heap.h"
#include "mem_sentry/reporter.h"
#include "mem_sentry/mem_sentry.h"
#include <iostream>

// A simple class to demonstrate allocation tracking
class DemoClass {
public:
    int x;
    DemoClass(int v) : x(v) { std::cout << "DemoClass constructed\n"; }
    ~DemoClass() { std::cout << "DemoClass destructed\n"; }
};

int main() {
    // 1. Create a reporter and a heap for tracking
    MEM_SENTRY::reporter::ConsoleReporter* reporter = new MEM_SENTRY::reporter::ConsoleReporter();
    MEM_SENTRY::heap::Heap* heap = new MEM_SENTRY::heap::Heap("DemoHeap");

    heap->SetReporter(reporter); // Attach reporter for logging

    // 2. Allocate a single object using placement new with the heap

    // Allocate a single object using heap pointer
    DemoClass* obj = new (heap) DemoClass(42);
    delete obj;

    // Allocate an array of objects using heap pointer
    DemoClass* arr = new (heap) DemoClass[3]{ {1}, {2}, {3} };
    delete[] arr;

    // All allocation/deallocation events are reported to the console
    return 0;
}
