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
#include <thread>
#include <alsa/asoundlib.h>

class AlsaInterface {
    static constexpr auto CONFIG_FILE = "./Resources/configs/audio.json";
//    Midi
    snd_seq_t *sequencer;
    int portID{};
    int npfd{};
    std::unique_ptr<pollfd> pfd;
    std::string midiClientName;
    std::unique_ptr<std::thread> midiThread;

    void initMidi();
    void beginPollMidi();
    void endPollMidi();
    [[nodiscard]] int getMidiClientId();

//    Audio
    snd_pcm_t* playback;
    std::atomic<bool> runningAudio = false, runningMidi = false;
    std::string playbackDeviceName;
    std::unique_ptr<std::thread> audioThread;

    void initAudio();
    void beginPlayback();
    void endPlayback();

public:
    AlsaInterface();
    ~AlsaInterface();
};
