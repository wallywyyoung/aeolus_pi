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

#ifdef LINUX

#pragma once

#include <functional>
#include <memory>
#include <thread>
#include <alsa/asoundlib.h>

#include "MemoryConstants.h"

class MidiData;

class AlsaInterface {
    //    General
    enum SampleFormat {
        S24 = SND_PCM_FORMAT_S24_3LE,
        S16 = SND_PCM_FORMAT_S16_LE
    };
    static constexpr auto SAMPLE_FORMAT = S24;
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
        std::function<void(float (&out)[PROCESS_SAMPLES_SIZE])> processAudio;
        bool runningAudio = false;
        snd_pcm_t* playback{};
    };
    alignas(CACHE_LINE_SIZE) AudioThreadObjects audioThreadObjects{};
    std::thread audioThread;

    //    Midi
    void initMidi(const std::string &clientName);
    void beginPollMidi();
    static void* midiHandler(const MidiThreadObjects *midiThreadObjects);
    void endPollMidi();
    [[nodiscard]] int getMidiClientId(const std::string &clientName);

    //    Audio
    void initAudio(const std::string &deviceName);
    void beginPlayback();
    static void audioHandler(const AudioThreadObjects * a);
    void endPlayback();

public:
    explicit AlsaInterface(std::function<void(float (&out)[PROCESS_SAMPLES_SIZE])> processAudio, std::function<void(const MidiData&)> submitMidi);
    ~AlsaInterface();
};

#endif
