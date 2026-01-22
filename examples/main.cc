#include <cstdlib>
#include <cstring>

#include "effect.h"
#include "audio.h"
#include "mem_sentry/heap.h"
#include "mem_sentry/reporter.h"


int main() {
    MEM_SENTRY::reporter::ConsoleReporter consoleReporter;
    MEM_SENTRY::heap::Heap* myEffectsHeap = new MEM_SENTRY::heap::Heap("Effects Heap");
    MEM_SENTRY::heap::Heap* myAudioHeap = new MEM_SENTRY::heap::Heap("Audio Heap");

    myEffectsHeap->SetReporter(&consoleReporter);
    myAudioHeap->SetReporter(&consoleReporter);

    Effect::setHeap(myEffectsHeap);
    Audio::setHeap(myAudioHeap);

    Effect *pEffect = new Effect();
    
    Audio *pAudio = new Audio();

    delete pEffect;
    
    delete pAudio;

    delete myEffectsHeap;
    
    delete myAudioHeap;

    return 0;
}