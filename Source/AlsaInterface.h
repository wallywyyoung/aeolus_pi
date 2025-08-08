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

#pragma once

#include <memory>
#include <alsa/asoundlib.h>
#include "MemoryUtilities.h"

class AlsaInterface {
    static constexpr auto CONFIG_FILE = "./Resources/configs/audio.json";

    static void createThread(pthread_t& tid, void* func, void* arg);

//    Midi
    struct MidiThreadObjects {
        bool runningMidi = false;
        snd_seq_t *sequencer{};
        int portID{};
        int npfd{};
        std::unique_ptr<pollfd> pfd;
    };
    alignas(CACHE_LINE_SIZE) MidiThreadObjects midiThreadObjects{};
    pthread_t midiTID;

    void initMidi(const std::string &clientName);
    void beginPollMidi();
    static void* midiHandler(void *stateStruct);
    void endPollMidi();
    [[nodiscard]] int getMidiClientId(const std::string &clientName);

//    Audio
    struct AudioThreadObjects {
        bool runningAudio = false;
        snd_pcm_t* playback{};
    };
    alignas(CACHE_LINE_SIZE) AudioThreadObjects audioThreadObjects{};
    pthread_t audioTID;

    void initAudio(const std::string &deviceName);
    void beginPlayback();
    static void* audioHandler(void* stateStruct);
    void endPlayback();

public:
    AlsaInterface();
    ~AlsaInterface();
};
