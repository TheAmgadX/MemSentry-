#include <iostream>
#include <cstdlib>
#include <cstring>

#include "effect.h"
#include "mem_sentry/heap.h"


int main() {
    MEM_SENTRY::heap::Heap* myEffectsHeap = new MEM_SENTRY::heap::Heap("SpecialEffectsHeap");

    Effect::SetHeap(myEffectsHeap);

    Effect *pEffect = new Effect();

    delete pEffect;

    delete myEffectsHeap;

    return 0;
}