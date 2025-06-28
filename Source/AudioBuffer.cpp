//
// Created by Wally Young on 6/7/25.
//

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
        std::transform(channel.begin(), channel.end(), channel.begin(), [&](float element) { return element * gain; });
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
