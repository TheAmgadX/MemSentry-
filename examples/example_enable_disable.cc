// Example 2: Enabling/Disabling the Library
// -------------------------------------------------------------
// This example shows how to control MemSentry's tracking using the
// MEM_SENTRY_ENABLE macro. When disabled, allocations are not tracked
// or reported. This is useful for production builds or performance testing.
//
// To disable MemSentry, define MEM_SENTRY_ENABLE 0 before including headers.
// -------------------------------------------------------------

#include "mem_sentry/heap.h"
#include "mem_sentry/reporter.h"
#include "mem_sentry/mem_sentry.h"
#include <iostream>

// Uncomment the following line to disable MemSentry for this file:
// #define MEM_SENTRY_ENABLE 0

// A simple class to demonstrate allocation
class Dummy {
public:
    Dummy() { std::cout << "Dummy constructed\n"; }
    ~Dummy() { std::cout << "Dummy destructed\n"; }
};

int main() {
#if MEM_SENTRY_ENABLE
    std::cout << "MemSentry ENABLED\n";
    // Allocations are tracked and reported
    MEM_SENTRY::reporter::ConsoleReporter reporter;
    MEM_SENTRY::heap::Heap* heap = new MEM_SENTRY::heap::Heap("EnableDisableHeap");

    heap->SetReporter(&reporter);

    Dummy* d = new (heap) Dummy();
    delete d;
#else
    std::cout << "MemSentry DISABLED\n";
    // Allocations are NOT tracked or reported
    Dummy* d = new Dummy();
    delete d;
#endif
    return 0;
}
