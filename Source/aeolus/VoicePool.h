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
// ---------------------------------------------------------------------------

#pragma once
#include "aeolus/Voice.h"

template <size_t MAX_VOICES = 128>
class VoicePool {
    std::array<Voice, MAX_VOICES> pool{ };
    std::vector<std::shared_ptr<Voice>> freeVoices;
public:
    VoicePool() {
        for (auto voice : pool) {
            freeVoices.push_back(std::make_shared<Voice>(voice));
        }
    }

    std::shared_ptr<Voice> getVoice(const PipeWave::State& state, const int stopIndex) {
        auto voice = freeVoices.back();
        freeVoices.pop_back();
        voice->init(state,stopIndex);
        return voice;
    }

    void releaseVoice(std::shared_ptr<Voice> &voice) {
        freeVoices.push_back(voice);
    }
};
