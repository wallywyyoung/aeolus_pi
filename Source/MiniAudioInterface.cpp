//
// Created by Wally Young on 8/26/25.
//

#include "MiniAudioInterface.h"

#include <stdexcept>
#include "MemoryConstants.h"
#include "aeolus/utilities/SimdUtilities.h"

MiniAudioInterface::MiniAudioInterface(std::function<void(float (&out)[PROCESS_SAMPLES_SIZE])> processAudio) : processAudio(processAudio) { }

void MiniAudioInterface::initAudio(const std::string &deviceName) {
    ma_device_config deviceConfig{};

    deviceConfig = ma_device_config_init(ma_device_type_playback);
    deviceConfig.playback.format = ma_format_f32;
    deviceConfig.playback.channels = OUTPUT_CHANNELS;
    deviceConfig.sampleRate = SAMPLE_RATE;
    deviceConfig.periodSizeInFrames = ALSA_PERIOD_SIZE;
    deviceConfig.dataCallback = audioHandler;
    deviceConfig.pUserData = &processAudio;

    if (ma_device_init(nullptr, &deviceConfig, &device) != MA_SUCCESS) {
        throw std::runtime_error("Failed to open playback device.");
    }
}
void MiniAudioInterface::beginPlayback() {
    if (ma_device_start(&device) != MA_SUCCESS) {
        printf("Failed to start playback device.\n");
        ma_device_uninit(&device);
        throw std::runtime_error("Failed to start playback device.");
    }
}

void MiniAudioInterface::audioHandler(ma_device *device, void *output, const void *input, ma_uint32 frameCount) {
    SimdUtilities::enableFlushToZero();
    alignas(CACHE_LINE_SIZE) static float fBuffer[PROCESS_SAMPLES_SIZE];
    if (frameCount < PROCESS_SAMPLES_SIZE) {
        return;
    }
    auto processAudio = static_cast<std::function<void(float(&out)[PROCESS_SAMPLES_SIZE])>*>(device->pUserData);
    (*processAudio)(*reinterpret_cast<float(*)[PROCESS_SAMPLES_SIZE]>(output));

    SimdUtilities::disableFlushToZero();
}

void MiniAudioInterface::endPlayback() {
    ma_device_uninit(&device);
}
