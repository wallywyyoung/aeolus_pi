//
// Created by Wally Young on 6/7/25.
//

#pragma once

#include <vector>

class AudioBuffer {
    int channels;
    int bufferSize;

    std::vector<std::vector<float>> audioBuffer;
protected:
    AudioBuffer() : channels{0}, bufferSize{0} { }

public:
    void setBuffer(std::vector<std::vector<float>> &newBuffer){
        audioBuffer = newBuffer;
    }

    void setBufferSize(std::size_t size) {
        bufferSize = size;
    }

    void zero();

    void applyGain(const float& gain);

    float* getWritePointer(int channel);
    [[nodiscard]] float* getReadPointer(int channel, int offset = 0) const;

    [[nodiscard]] int getNumSamples() const { return /*sampleRate * */ bufferSize; }
    [[nodiscard]] int getNumChannels() const { return channels; }

    AudioBuffer(int channels, int bufferSize);

    void addFrom(int toChannel, int toStartOffset, const AudioBuffer& from, int fromChannel, int fromStartOffset, int sampleCount);

    void clear();
};
