//
// Created by Wally Young on 8/3/25.
//

#pragma once
#include <cstdint>

#include "aeolus/globals.h"

class MemoryUtilities {
    public:
    // Should be 64 on Raspberry Pi 4B. Also consider std::hardware_destructive_interference_size
    static constexpr unsigned short CACHE_LINE_SIZE = 64;
    // Audio Processing Buffer
    static constexpr unsigned short AUDIO_CHANNEL_BUFFER_SIZE = 1024;
    static constexpr unsigned short AUDIO_BUFFER_SIZE = AUDIO_CHANNEL_BUFFER_SIZE * N_OUTPUT_CHANNELS;
    // Using inline assembly for ARMv8-a enabling flush-to-zero
    // Denormals are handled differently in ARMv8-a so there is no equivalent denormals-are-zero
    static void enableFlushToZero();
    static void disableFlushToZero();
    // Ensure your in and out buffers are aligned to 16 to be NEON compliant.
    static void ConvertF32toS16(float (&in)[AUDIO_BUFFER_SIZE], std::int16_t* out);
};
