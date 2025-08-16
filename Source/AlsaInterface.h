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

#include <functional>
#include <memory>
#include <thread>
#include <alsa/asoundlib.h>

#include "MemoryUtilities.h"

class MidiData;

class AlsaInterface {
    //    General
    static constexpr auto CONFIG_FILE = "./Resources/configs/audio.json";

    //    Midi
    struct MidiThreadObjects {
        std::function<void(const MidiData&)> submitMidiEvent;
        bool runningMidi = false;
        snd_seq_t *sequencer{};
        int portID{};
        int npfd{};
        std::unique_ptr<pollfd> pfd;
    };
    alignas(CACHE_LINE_SIZE) MidiThreadObjects midiThreadObjects{};
    std::thread midiThread;

    //    Audio
    struct AudioThreadObjects {
        std::function<void(float (&out)[NUMBER_SAMPLES])> processAudio;
        bool runningAudio = false;
        snd_pcm_t* playback{};
    };
    alignas(CACHE_LINE_SIZE) AudioThreadObjects audioThreadObjects{};
    std::thread audioThread;

    //    General
    void init();
    static void createThread(pthread_t& tid, void* func, void* arg);

    //    Midi
    void initMidi(const std::string &clientName);
    void beginPollMidi();
    static void* midiHandler(MidiThreadObjects *midiThreadObjects);
    void endPollMidi();
    [[nodiscard]] int getMidiClientId(const std::string &clientName);

    //    Audio
    void initAudio(const std::string &deviceName);
    void beginPlayback();
    static void audioHandler(AudioThreadObjects* a);
    void endPlayback();

public:
    explicit AlsaInterface(std::function<void(float (&out)[NUMBER_SAMPLES])> processAudio, std::function<void(const MidiData&)> submitMidi);
    ~AlsaInterface();
};
