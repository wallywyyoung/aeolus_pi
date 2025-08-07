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

#include "MemoryUtilities.h"
#include <vector>

class AudioBuffer {
protected:
    int channels;
    std::size_t bufferSize{};
    alignas (CACHE_LINE_SIZE) std::vector<float> audioBuffer{};

public:
    explicit AudioBuffer() = delete;
    AudioBuffer(const int channels, const int bufferSize) : channels(channels), bufferSize(bufferSize), audioBuffer(channels * bufferSize, 0.0f) { }

    void setBuffer(const std::vector<float> &newBuffer){ audioBuffer = newBuffer; }

    void applyGain(const float& gain);

    float* getWritePointer(const int channel) { return &audioBuffer[channel * bufferSize]; }

    [[nodiscard]] const float *getReadPointer(const int channel, const int offset = 0) const  { return &audioBuffer[channel * bufferSize + offset]; }

    [[nodiscard]] std::size_t getNumSamples() const { return bufferSize; }

    [[nodiscard]] int getNumChannels() const { return channels; }

    void addFrom(int toChannel, int toStartOffset, const AudioBuffer& from, int fromChannel, int fromStartOffset, int sampleCount);

    void clear();
};
