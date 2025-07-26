// ----------------------------------------------------------------------------
//
//  Copyright (C) 2025 Wally Young <wallywyyoung@users.noreply.github.com>
//  Copyright (C) 2021 Arthur Benilov <arthur.benilov@gmail.com>
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

#include "AudioBuffer.h"

#include <string>
#include <vector>
#include <AudioFile/AudioFile.h>

class IR : public AudioBuffer {
    std::string name;

public:
    explicit IR() = delete;

    IR(const std::string &name, const AudioFile<float> audioFile, const int startOffset = 0) : AudioBuffer(audioFile.getNumChannels(), audioFile.getNumSamplesPerChannel()), name(name), zeroDelay(false) {
        for (int i = 0; i < getNumChannels(); ++i) {
            std::copy(audioFile.samples[i].begin() + startOffset, audioFile.samples[i].end(), audioBuffer.begin() + (i * bufferSize));
        }
    }

    IR(const IR& other) = default;

    bool zeroDelay;

    void clear() {
        name.clear();
        AudioBuffer::clear();
    }
};

struct IRs {
    std::vector<IR> irs{};
    int longestIRLength{};
};