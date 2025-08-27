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
#include "AlsaInterface.h"
#include "EngineGlobal.h"

#include "MiniAudioInterface.h"

bool running = true;

void signalHandler(const int signal) {
    std::cout << "Aeolus got signal " << signal << std::endl;
    running = false;
    exit(signal);
}

int main (int, char*[]) {
    std::signal(SIGINT | SIGTERM | SIGSEGV | SIGABRT | SIGFPE | SIGILL | SIGBUS, signalHandler);
    const auto* engineGlobal = new EngineGlobal();
#ifdef LINUX
    const auto* audioInterface = new AlsaInterface(
        std::bind(&EngineGlobal::process, const_cast<EngineGlobal*>(engineGlobal), std::placeholders::_1),
        std::bind(&EngineGlobal::pushMidi, const_cast<EngineGlobal*>(engineGlobal), std::placeholders::_1));
#elifdef MACOS
    const auto* audioInterface = new MiniAudioInterface(std::bind(&EngineGlobal::process, const_cast<EngineGlobal*>(engineGlobal), std::placeholders::_1));
#endif
    std::cout << "Aeolus is Ready" << std::endl;
    do {
        std::this_thread::yield();
    } while(running);
    std::cout << "Aeolus is Closing" << std::endl;
    delete audioInterface;
}
