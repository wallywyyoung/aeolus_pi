// ----------------------------------------------------------------------------
//
//  Copyright (C) 2025 Wally Young <wallywyyoung@users.noreply.github.com>
//  Copyright (C) 2021 Arthur Benilov <arthur.benilov@gmail.com>
//  Copyright (C) 2003-2013 Fons Adriaensen <fons@linuxaudio.org>
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 3 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
// ----------------------------------------------------------------------------

#include "AlsaInterface.h"
#include "aeolus/EngineGlobal.h"
#include <csignal>

bool running = true;
std::unique_ptr<AlsaInterface> midiInterface;
intptr_t fpsr;

// Using inline assembly for ARMv7/ARMv8 enabling of denormals
// TODO: Check this is being done right.
void enableFlushToZeroDenormalsAreZero() {
    intptr_t ftz = (1 << 24 /* FZ */);
    intptr_t daz = (1 << 25 /* FZ */);
    asm volatile("mrs %0, fpcr" : "=r"(fpsr));
    asm volatile("msr fpcr, %0" : : "ri"(fpsr | ftz | daz));
//    asm volatile("vmrs %0, fpscr" : "=r"(fpsr));
//    asm volatile("vmsr fpscr, %0" : : "ri"(fpsr | ftz | daz));
}

void disableFlushToZeroDenormalsAreZero() {
    asm volatile("msr fpcr, %0" : : "ri"(fpsr));
//    asm volatile("vmsr fpscr, %0" : : "ri"(fpsr));
}

void signalHandler(int signal) {
    running = false;
    midiInterface->endPlayback();
    midiInterface->endPollMidi();
    disableFlushToZeroDenormalsAreZero();
    exit(signal);
}

int main (int argc, char* argv[]) {
    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);
    std::signal(SIGSEGV, signalHandler);
    std::signal(SIGABRT, signalHandler);
    std::signal(SIGFPE, signalHandler);
    std::signal(SIGILL, signalHandler);
    std::signal(SIGBUS, signalHandler);

    enableFlushToZeroDenormalsAreZero();

    midiInterface = std::make_unique<AlsaInterface>();
    auto engine = EngineGlobal();

    midiInterface->beginPollMidi();
    midiInterface->beginPlayback();

    do {
        sleep(1);
    } while(running);
}
