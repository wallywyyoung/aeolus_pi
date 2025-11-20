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

#include <algorithm>
#include <array>
#include "MemoryConstants.h"
#include "aeolus/utilities/SimdUtilities.h"

template <std::size_t SIZE, std::size_t CHANNELS>
class StaticAudioBuffer {
    alignas (CACHE_LINE_SIZE) std::array<float, SIZE * CHANNELS> audioBuffer{};
public:
    StaticAudioBuffer() = default;
    ~StaticAudioBuffer() = default;

    float* getWritePointer(const int channel) { return &audioBuffer[channel * SIZE]; }
    [[nodiscard]] const float* getReadPointer(const int channel, const int offset = 0) const  { return &audioBuffer[channel * SIZE + offset]; }
    void addFrom(const StaticAudioBuffer &from) { SimdUtilities::add(audioBuffer.data(), from.audioBuffer.data(), SIZE * CHANNELS); }
    void clear()  { audioBuffer.fill(0.0f); }
};
