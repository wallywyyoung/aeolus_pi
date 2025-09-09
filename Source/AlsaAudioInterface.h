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

#ifdef LINUX

#include <functional>
#include <thread>
#include <alsa/asoundlib.h>
#include "MemoryConstants.h"

class MidiData;

class AlsaAudioInterface {
    //    General
    enum SampleFormat {
        S24 = SND_PCM_FORMAT_S24_3LE,
        S16 = SND_PCM_FORMAT_S16_LE
    };
    static constexpr auto SAMPLE_FORMAT = S24;
    static constexpr auto CONFIG_FILE = "./Resources/configs/audio.json";

    struct AudioThreadObjects {
        std::function<void(float (&out)[PROCESS_SAMPLES_SIZE])> processAudio;
        bool runningAudio = false;
        snd_pcm_t* playback{};
    };
    alignas(CACHE_LINE_SIZE) AudioThreadObjects audioThreadObjects{};
    std::thread audioThread;

    void initAudio(const std::string &deviceName);
    void beginPlayback();
    static void audioHandler(const AudioThreadObjects * a);
    void endPlayback();

public:
    explicit AlsaAudioInterface(std::function<void(float (&out)[PROCESS_SAMPLES_SIZE])> processAudio);
    ~AlsaAudioInterface();
};

#endif
