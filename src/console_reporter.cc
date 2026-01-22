#include "mem_sentry/reporter.h"
#include "mem_sentry/heap.h"
#include <iostream>
#include <iomanip>


void MEM_SENTRY::reporter::ConsoleReporter::onAlloc(alloc_header::AllocHeader* alloc) {
    if (!alloc || !alloc->p_Heap) return;

    const char* CLR_BORDER = "\033[36m";   // Cyan
    const char* CLR_LABEL  = "\033[1;37m"; // Bold White
    const char* CLR_VAL    = "\033[33m";   // Yellow
    const char* CLR_RESET  = "\033[0m";
    const char* CLR_EVENT  = "\033[1;32m"; // Bold Green (ALLOC)

    int size = alloc->m_Size + alloc->m_Alignment;

    std::cout << CLR_BORDER
              << "╔══════════════════════ ALLOCATION ══════════════════════╗"
              << CLR_RESET << "\n";

    std::printf("%s║%s Event:          %sALLOC%s                                   %s║%s\n",
        CLR_BORDER, CLR_EVENT, CLR_EVENT, CLR_RESET, CLR_BORDER, CLR_RESET);

    std::printf("%s║%s Heap:           %s%-38s %s║%s\n",
        CLR_BORDER, CLR_LABEL, CLR_VAL,
        alloc->p_Heap->GetName(), CLR_BORDER, CLR_RESET);

    std::printf("%s║%s Size:           %s%-6d bytes (Align: %-2d)        %s║%s\n",
        CLR_BORDER, CLR_LABEL, CLR_VAL,
        alloc->m_Size, alloc->m_Alignment,
        CLR_BORDER, CLR_RESET);

    std::printf("%s║%s Heap Total:     %s%-38d %s║%s\n",
        CLR_BORDER, CLR_LABEL, CLR_VAL,
        alloc->p_Heap->GetTotal(), CLR_BORDER, CLR_RESET);

    std::cout << CLR_BORDER
              << "╚═════════════════════════════════════════════════════════╝"
              << CLR_RESET << "\n";
}

void MEM_SENTRY::reporter::ConsoleReporter::onDealloc(alloc_header::AllocHeader* alloc) {
    if (!alloc || !alloc->p_Heap) return;

    const char* CLR_BORDER = "\033[36m";   // Cyan
    const char* CLR_LABEL  = "\033[1;37m"; // Bold White
    const char* CLR_VAL    = "\033[33m";   // Yellow
    const char* CLR_RESET  = "\033[0m";
    const char* CLR_EVENT  = "\033[1;31m"; // Bold Red (DEALLOC)

    int size = alloc->m_Size + alloc->m_Alignment;

    std::cout << CLR_BORDER
              << "╔════════════════════ DEALLOCATION ═════════════════════╗"
              << CLR_RESET << "\n";

    std::printf("%s║%s Event:          %sFREE%s                                    %s║%s\n",
        CLR_BORDER, CLR_EVENT, CLR_EVENT, CLR_RESET, CLR_BORDER, CLR_RESET);

    std::printf("%s║%s Heap:           %s%-38s %s║%s\n",
        CLR_BORDER, CLR_LABEL, CLR_VAL,
        alloc->p_Heap->GetName(), CLR_BORDER, CLR_RESET);

    std::printf("%s║%s Freed:          %s%-6d bytes (Align: %-2d)        %s║%s\n",
        CLR_BORDER, CLR_LABEL, CLR_VAL,
        alloc->m_Size, alloc->m_Alignment,
        CLR_BORDER, CLR_RESET);

    std::printf("%s║%s Heap Total:     %s%-38d %s║%s\n",
        CLR_BORDER, CLR_LABEL, CLR_VAL,
        alloc->p_Heap->GetTotal(), CLR_BORDER, CLR_RESET);

    std::cout << CLR_BORDER
              << "╚═════════════════════════════════════════════════════════╝"
              << CLR_RESET << "\n";
}

void MEM_SENTRY::reporter::ConsoleReporter::report(alloc_header::AllocHeader* p_Alloc) {
    if (!p_Alloc) return;

    // Colors: Cyan for structure, Yellow for data, Red for the "Header" title
    const char* CLR_HEADER = "\033[1;35m"; // Bold Magenta
    const char* CLR_BORDER = "\033[36m";   // Cyan
    const char* CLR_LABEL  = "\033[1;37m"; // Bold White
    const char* CLR_VAL    = "\033[33m";   // Yellow
    const char* CLR_RESET  = "\033[0m";

    std::cout << CLR_BORDER << "╔══════════════════ MEMORY BLOCK REPORT ══════════════════╗" << CLR_RESET << "\n";
    
    // Header Info
    std::printf("%s║%s %-15s %s%-38d %s║%s\n", 
        CLR_BORDER, CLR_LABEL, "Allocation ID:", CLR_VAL, p_Alloc->m_AllocId, CLR_BORDER, CLR_RESET);
    
    std::printf("%s║%s %-15s %s0x%-36X %s║%s\n", 
        CLR_BORDER, CLR_LABEL, "Signature:", CLR_VAL, p_Alloc->m_Signature, CLR_BORDER, CLR_RESET);

    // Heap Info
    const char* heapName = p_Alloc->p_Heap ? p_Alloc->p_Heap->GetName() : "ORPHANED/UNKNOWN";
    std::printf("%s║%s %-15s %s%-38s %s║%s\n", 
        CLR_BORDER, CLR_LABEL, "Heap Name:", CLR_VAL, heapName, CLR_BORDER, CLR_RESET);

    std::cout << CLR_BORDER << "╠----------------------------------------------------------╣" << CLR_RESET << "\n";

    // Size Info
    std::printf("%s║%s %-15s %s%-6d bytes %s(Align: %-2d)               %s║%s\n", 
        CLR_BORDER,             // 1 (%s)
        CLR_LABEL,              // 2 (%s)
        "User Size:",           // 3 (%-15s)
        CLR_VAL,                // 4 (%s)
        p_Alloc->m_Size,        // 5 (%-6d)
        CLR_LABEL,              // 6 (%s) - NEW: Added this to fix the count
        p_Alloc->m_Alignment,   // 7 (%-2d)
        CLR_BORDER,             // 8 (%s)
        CLR_RESET               // 9 (%s)
    );

    // Address Info
    std::printf("%s║%s %-15s %s%p                           %s║%s\n", 
        CLR_BORDER, CLR_LABEL, "Raw Address:", CLR_VAL, p_Alloc->p_OriginalAddress, CLR_BORDER, CLR_RESET);

    // Footer with Heap Total
    if (p_Alloc->p_Heap) {
        std::cout << CLR_BORDER << "╠----------------------------------------------------------╣" << CLR_RESET << "\n";
        std::printf("%s║%s %-15s %s%-31d bytes %s║%s\n", 
            CLR_BORDER, CLR_LABEL, "Heap Total Now:", CLR_VAL, p_Alloc->p_Heap->GetTotal(), CLR_BORDER, CLR_RESET);
    }

    std::cout << CLR_BORDER << "╚══════════════════════════════════════════════════════════╝" << CLR_RESET << "\n" << std::endl;
}
