//
// Created by Wally Young on 8/26/25.
//

#pragma once

#include <functional>

extern "C" {
#include <miniaudio.h>
}
#include <string>
#include "MemoryConstants.h"

class MiniAudioInterface final {
public:
    ~MiniAudioInterface() = default;
    explicit MiniAudioInterface(std::function<void(float (&out)[PROCESS_SAMPLES_SIZE])> processAudio);

private:
    std::function<void(float (&out)[PROCESS_SAMPLES_SIZE])> processAudio;
    ma_device device;

    void initAudio(const std::string &deviceName);
    void beginPlayback();
    static void audioHandler(ma_device* device, void* output, const void* input, ma_uint32 frameCount);
    void endPlayback();
};
