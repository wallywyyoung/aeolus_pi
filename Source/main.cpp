
#include "AlsaInterface.h"
#include "AeolusAudioProcessor.h"

int main (int argc, char* argv[]) {
    bool running = true;
    auto midiInterface = AlsaInterface();
    auto aeolusAudioProcessor = AeolusAudioProcessor();
    do {
        midiInterface.poll();
    } while(running);
}



