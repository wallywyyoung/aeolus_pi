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

#include "AudioBuffer.h"

#include <algorithm>

float* AudioBuffer::getWritePointer(int channel) {
    return audioBuffer[channel].data();
}

float* AudioBuffer::getReadPointer(int channel, int offset) const {
    return const_cast<float*>(audioBuffer[channel].data() + offset);
}

void AudioBuffer::clear() {
    for (auto& channel : audioBuffer) {
        channel.assign(channel.size(), 0);
    }
}

void AudioBuffer::zero() {
    for (auto& channel : audioBuffer) {
        channel.assign(channel.size(), 0.0f);
    }
}

void AudioBuffer::applyGain(const float& gain) {
    for (auto& channel : audioBuffer) {
        std::ranges::transform(channel, channel.begin(), [&](float element) { return element * gain; });
    }
}

void AudioBuffer::addFrom(int toChannel, int toStartOffset, const AudioBuffer &from, int fromChannel, int fromStartOffset, int sampleCount) {
    auto toChannelStart = audioBuffer[toChannel].data() + toStartOffset;
    auto fromChannelStart = from.getReadPointer(fromChannel)  + fromStartOffset;
    std::transform(fromChannelStart, fromChannelStart + sampleCount, toChannelStart, toChannelStart, std::plus());
}

AudioBuffer::AudioBuffer(int channels, int bufferSize) : bufferSize(bufferSize), channels(channels) {
    audioBuffer.reserve(channels);
    for (auto& channel : audioBuffer) {
        channel.reserve(bufferSize);
    }
}
