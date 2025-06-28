
#include "AlsaInterface.h"
#include "aeolus/EngineGlobal.h"

// Using inline assembly for ARMv7/ARMv8 enabling of denormals
// TODO: Check this is being done right.
void enableFlushToZeroDenormalsAreZero(intptr_t& fpsr) {
    intptr_t ftz = (1 << 24 /* FZ */);
    intptr_t daz = (1 << 25 /* FZ */);
    asm volatile("mrs %0, fpcr" : "=r"(fpsr));
    asm volatile("msr fpcr, %0" : : "ri"(fpsr | ftz | daz));
//    asm volatile("vmrs %0, fpscr" : "=r"(fpsr));
//    asm volatile("vmsr fpscr, %0" : : "ri"(fpsr | ftz | daz));
}

void disableFlushToZeroDenormalsAreZero(intptr_t& fpsr) {
    asm volatile("msr fpcr, %0" : : "ri"(fpsr));
//    asm volatile("vmsr fpscr, %0" : : "ri"(fpsr));
}

int main (int argc, char* argv[]) {
    intptr_t fpsr;
    enableFlushToZeroDenormalsAreZero(fpsr);

    auto midiInterface = AlsaInterface();
    aeolus::EngineGlobal::getInstance();

    midiInterface.beginPollMidi();
    midiInterface.beginPlayback();

    bool running = true;
    do {
        sleep(1);
    } while(running);

    midiInterface.endPlayback();
    midiInterface.endPollMidi();

    disableFlushToZeroDenormalsAreZero(fpsr);
}
