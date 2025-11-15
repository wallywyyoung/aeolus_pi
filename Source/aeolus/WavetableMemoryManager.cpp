//
// Created by Wally Young on 11/14/25.
//

#include "aeolus/WavetableMemoryManager.h"
#include <cassert>

alignas(CACHE_LINE_SIZE) float WavetableMemoryManager::wavetables[WAVETABLE_MEMORY_SIZE];
size_t WavetableMemoryManager::unusedIndex = 0;
std::mutex WavetableMemoryManager::mutex;

float *WavetableMemoryManager::allocateWavetable(const size_t wavetableSize) {
    std::lock_guard lock(mutex);
    assert(unusedIndex + wavetableSize < WAVETABLE_MEMORY_SIZE);
    auto *wavetable = &wavetables[unusedIndex];
    unusedIndex += (wavetableSize + CACHE_LINE_FLOATS - 1) & ~(CACHE_LINE_FLOATS - 1);
    return wavetable;
}
