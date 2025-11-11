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
#include "AlsaMidiInterface.h"
#endif
#include "AlsaAudioInterface.h"
#include "RtAudioInterface.h"
#include "RestManager.h"

bool running = true;

void signalHandler(const int signal) {
    running = false;
    exit(signal);
}

int main (int, char*[]) {
    std::signal(SIGTERM, signalHandler);
    std::signal(SIGINT, signalHandler);
    auto engineGlobal = new EngineGlobal();
#ifdef LINUX
    const auto midiInterface = new AlsaMidiInterface([engineGlobal](const MidiData& midiData){ engineGlobal->pushMidi(midiData); });
    const auto audioInterface = new AlsaAudioInterface([engineGlobal](float (&out)[PROCESS_SAMPLES_SIZE]){ engineGlobal->process(out); });
#elifdef MACOS
    const auto audioInterface = new RtAudioInterface([engineGlobal](float (&out)[PROCESS_SAMPLES_SIZE]){ engineGlobal->process(out); });
#endif
    RestManager::runServer(engineGlobal->getOrgan());
    delete audioInterface;
#ifdef LINUX
    delete midiInterface;
#endif
    delete engineGlobal;
}
