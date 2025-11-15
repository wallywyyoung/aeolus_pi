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
// ---------------------------------------------------------------------------

#pragma once
#include "aeolus/Voice.h"

template <size_t MAX_VOICES = 128>
class VoicePool {
    std::array<Voice, MAX_VOICES> pool{ };
    std::vector<Voice*> freeVoices;
public:
    VoicePool() {
        for (auto voice : pool) {
            freeVoices.push_back(&voice);
        }
    }

    Voice* getVoice(const std::shared_ptr<StaticPipe> &pipeWave, const float &outputGain, const float &chiffGain, const int stopIndex) {
        const auto voice = freeVoices.back();
        freeVoices.pop_back();
        voice->init(pipeWave, outputGain, chiffGain, stopIndex);
        return voice;
    }

    void releaseVoice(Voice* &voice) {
        freeVoices.push_back(voice);
    }
};
