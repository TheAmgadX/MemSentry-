// Example 3: MemoryReport from Heap
// -------------------------------------------------------------
// This example demonstrates how to use Heap::ReportMemory to print
// allocation information between two allocation IDs. This is useful
// for leak detection and memory usage snapshots.
// -------------------------------------------------------------

#include "mem_sentry/heap.h"
#include "mem_sentry/reporter.h"
#include "mem_sentry/mem_sentry.h"
#include <iostream>

// A simple class for demonstration
class Foo {
public:
    Foo() { std::cout << "Foo constructed\n"; }
    ~Foo() { std::cout << "Foo destructed\n"; }
};

int main() {
    MEM_SENTRY::reporter::ConsoleReporter reporter;
    MEM_SENTRY::heap::Heap* heap = new MEM_SENTRY::heap::Heap("ReportHeap");
    heap->SetReporter(&reporter);

    // Mark the start allocation ID
    int startId = heap->GetNextId();
    Foo* a = new (heap) Foo();
    Foo* b = new (heap) Foo();
    // Mark the end allocation ID
    int endId = heap->GetNextId() - 1;

    // Report all allocations between startId and endId
    heap->ReportMemory(startId, endId);

    // Clean up
    delete a;
    delete b;
    return 0;
}
