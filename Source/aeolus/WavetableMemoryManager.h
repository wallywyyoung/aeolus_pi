//
// Created by Wally Young on 11/14/25.
//

#pragma once

#include <mutex>
#include "MemoryConstants.h"

class WavetableMemoryManager {
    constexpr static size_t WAVETABLE_MEMORY_SIZE = 33554432;
    constexpr static size_t CACHE_LINE_FLOATS = CACHE_LINE_SIZE / sizeof(float);
    alignas(CACHE_LINE_SIZE) static float wavetables[WAVETABLE_MEMORY_SIZE]; // static float array is guaranteed to be initialized to 0
    static size_t unusedIndex;
    static std::mutex mutex;

    WavetableMemoryManager() = default;
    WavetableMemoryManager(const WavetableMemoryManager&) = delete;
    WavetableMemoryManager& operator=(const WavetableMemoryManager&) = delete;

public:
    static WavetableMemoryManager & getInstance() {
        static WavetableMemoryManager instance;
        return instance;
    }

    static float* allocateWavetable(size_t wavetableSize);
};
