//
// Created by Wally Young on 6/7/25.
//

#ifndef AEOLUS_PI_AUDIOBUFFER_H
#define AEOLUS_PI_AUDIOBUFFER_H

#include <vector>

class AudioBuffer {
    const int sampleRate;
    const int bufferSize;
    const int channels;

    std::vector<std::vector<float>> audioBuffer;

public:
    AudioBuffer(int sampleRate, int bufferSize, int channels);

    float* getWritePointer(int channel);
    [[nodiscard]] float* getReadPointer(int channel) const;

    [[nodiscard]] int getNumSamples() const { return sampleRate * bufferSize; }
    [[nodiscard]] int getNumChannels() const { return channels; }

    void addFrom(int toChannel, int toStartOffset, const AudioBuffer& from, int fromChannel, int fromStartOffset, int sampleCount);

    void clear();
};


#endif //AEOLUS_PI_AUDIOBUFFER_H
