#pragma once
#include "mem_sentry/alloc_header.h"

// Forward declaration: We don't include heap.h here!
namespace MEM_SENTRY::heap { class Heap; }

namespace MEM_SENTRY::reporter {       
    class IReporter {
    public:
        virtual ~IReporter() = default;
        virtual void onAlloc(alloc_header::AllocHeader* alloc) = 0;
        virtual void onDealloc(alloc_header::AllocHeader* alloc) = 0;
        virtual void report(alloc_header::AllocHeader* alloc) = 0;
    };

    class ConsoleReporter : public IReporter {
    public:
        // We only declare the methods here.
        virtual void onAlloc(alloc_header::AllocHeader* alloc) override;
        virtual void onDealloc(alloc_header::AllocHeader* alloc) override;
        virtual void report(alloc_header::AllocHeader* alloc) override;
    };
}