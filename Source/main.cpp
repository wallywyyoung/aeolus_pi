
#include "AlsaInterface.h"
#include "AeolusAudioProcessor.h"
#include "aeolus/addsynth.h"
#include "aeolus/EngineGlobal.h"
#include "IOManager.h"
#include <arm_neon.h>

// Using inline assembly for ARMv7/ARMv8 enabling of denormals
void enable_ftz_daz() {
    unsigned int fpscr;
    asm volatile("vmrs %0, fpscr" : "=r" (fpscr)); // Read FPSCR
    fpscr |= (1 << 24); // Set FZ bit (Flush-to-Zero)
    fpscr |= (1 << 25); // Set DN bit (Default NaN, implies DAZ on some architectures)
    asm volatile("vmsr fpscr, %0" : : "r" (fpscr)); // Write FPSCR
}

int main (int argc, char* argv[]) {
    enable_ftz_daz();
    bool running = true;
    auto midiInterface = AlsaInterface();
    auto aeolusAudioProcessor = AeolusAudioProcessor();
    auto ioManager = aeolus::IOManager();

    aeolus::EngineGlobal::getInstance().addIRs(ioManager.loadIRs());
    auto synths = ioManager.loadPipes();
    aeolus::EngineGlobal::getInstance().addSynths(synths);
    aeolus::Model::getInstance().addSynths(synths);

    do {
        midiInterface.poll();
    } while(running);
}



