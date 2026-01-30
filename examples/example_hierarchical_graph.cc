// Example 4: Hierarchical Graph Reporting
// -------------------------------------------------------------
// This example shows how to connect multiple heaps and report total
// memory usage across the hierarchy. Demonstrates HeapFactory::ConnectHeaps
// and Heap::GetTotalHH for hierarchical memory tracking.
// -------------------------------------------------------------

#include "mem_sentry/heap.h"
#include "mem_sentry/reporter.h"
#include "mem_sentry/mem_sentry.h"

#include <iostream>


// A simple class for demonstration
class Bar {
public:
    Bar() { std::cout << "Bar constructed\n"; }
    ~Bar() { std::cout << "Bar destructed\n"; }
};

int main() {
    MEM_SENTRY::reporter::ConsoleReporter reporter;
    MEM_SENTRY::heap::Heap* heapA = new MEM_SENTRY::heap::Heap("HeapA");
    MEM_SENTRY::heap::Heap* heapB = new MEM_SENTRY::heap::Heap("HeapB");
    MEM_SENTRY::heap::Heap* heapC = new MEM_SENTRY::heap::Heap("HeapC");

    heapA->SetReporter(&reporter);
    heapB->SetReporter(&reporter);
    heapC->SetReporter(&reporter);

    // Connect heaps into a hierarchy (A <-> B <-> C)
    MEM_SENTRY::heap::HeapFactory::ConnectHeaps(heapA, heapB);
    MEM_SENTRY::heap::HeapFactory::ConnectHeaps(heapB, heapC);

    // Allocate objects in different heaps
    Bar* a = new (heapA) Bar();
    Bar* b = new (heapB) Bar();
    Bar* c = new (heapC) Bar();

    // Report total memory usage across the hierarchy
    std::cout << "Total memory in hierarchy: " << heapA->GetTotalHH() << " bytes\n";

    // Clean up
    delete a;
    delete b;
    delete c;
    return 0;
}
