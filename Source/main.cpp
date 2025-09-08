// ----------------------------------------------------------------------------
//
//  Copyright (C) 2025 Wally Young <wallywyyoung@users.noreply.github.com>
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

#include <csignal>
#include <functional>
#include <thread>
#include "EngineGlobal.h"

#ifdef LINUX
#include "AlsaInterface.h"
#endif

#ifdef MACOS
#include "RtAudioInterface.h"
#endif

bool running = true;

void signalHandler(const int signal) {
    running = false;
    exit(signal);
}

int main (int, char*[]) {
    std::signal(SIGINT | SIGTERM | SIGSEGV | SIGABRT | SIGFPE | SIGILL | SIGBUS, signalHandler);
    auto engineGlobal = new EngineGlobal();
#ifdef LINUX
    const auto audioInterface = new AlsaInterface(
        [engineGlobal](float (&out)[PROCESS_SAMPLES_SIZE]){ engineGlobal->process(out); },
        [engineGlobal](const MidiData& midiData){ engineGlobal->pushMidi(midiData); });
#elifdef MACOS
    const auto audioInterface = new RtAudioInterface([engineGlobal](float (&out)[PROCESS_SAMPLES_SIZE]){ engineGlobal->process(out); });
#endif
    std::cout << "Aeolus is Ready" << std::endl;
    do {
        std::this_thread::yield();
    } while(running);
    std::cout << "Aeolus is Closing" << std::endl;
    delete audioInterface;
    delete engineGlobal;
}
