// Example 6: Interfaces and Strategy Pattern
// -------------------------------------------------------------
// This example demonstrates how to use the IReporter interface and
// inject custom reporting strategies. Shows how to implement a custom
// reporter for memory events and use it with a Heap.
// -------------------------------------------------------------

#include "mem_sentry/heap.h"
#include "mem_sentry/reporter.h"
#include "mem_sentry/mem_sentry.h"
#include "mem_sentry/sentry.h"

#include <iostream>

// Custom reporter that logs memory events to std::printf()
class CustomReporter : public MEM_SENTRY::reporter::IReporter {
public:
    void onAlloc(MEM_SENTRY::alloc_header::AllocHeader* alloc) override {
        std::printf("[CustomReporter] Allocation ID: %d, Size: %d\n", alloc->m_AllocId, alloc->m_Size);
    }
    void onDealloc(MEM_SENTRY::alloc_header::AllocHeader* alloc) override {
        std::printf("[CustomReporter] Deallocation ID: %d\n", alloc->m_AllocId);
    }
    void report(MEM_SENTRY::alloc_header::AllocHeader* alloc) override {
        std::printf("[CustomReporter] Report for ID: %d, Size: %d\n", alloc->m_AllocId, alloc->m_Size);
    }
};

// Widget inherits from ISentry<> to enable sentry features (e.g., tracking, hooks)
class Widget : public MEM_SENTRY::sentry::ISentry<Widget> {
public:
    Widget() { std::cout << "Widget constructed\n"; }
    ~Widget() { std::cout << "Widget destructed\n"; }
};


int main() {
    // Use a custom reporter for memory events
    CustomReporter reporter;
    MEM_SENTRY::heap::Heap* heap = new MEM_SENTRY::heap::Heap("StrategyHeap");
    Widget::setHeap(heap); // set specfic heap to the class.
    heap->SetReporter(&reporter);

    // Allocate Widget using heap pointer (placement new)
    // ISentry hooks will be called automatically if implemented
    Widget* w = new Widget(); // no need to write which heap to use now.

    delete w; // ISentry dealloc hook called

    // Clean up heap
    delete heap;
    return 0;
}
