//
// Created by Wally Young on 11/14/25.
//

#pragma once

#include <random>
#include "MemoryConstants.h"

class RandomTable {
    constexpr static size_t TABLE_MEMORY_SIZE = 65536;
    constexpr static size_t CACHE_LINE_FLOATS = CACHE_LINE_SIZE / sizeof(float);
    alignas(CACHE_LINE_SIZE) float randomTable[TABLE_MEMORY_SIZE]{}; // static float array is guaranteed to be initialized to 0
    size_t readIndex;

    RandomTable() {
        thread_local std::random_device rnd;
        thread_local std::mt19937 gen(rnd());
        thread_local std::uniform_real_distribution dist(-1.0f, 1.0f);
        readIndex = 0;
        for (auto& random : randomTable) {
            random = dist(gen);
        }
    }

public:
    RandomTable(const RandomTable&) = delete;
    RandomTable& operator=(const RandomTable&) = delete;

    static RandomTable & getInstance() {
        static RandomTable instance;
        return instance;
    }

    template<size_t NUMBER_OF_RANDOMS>
    float* getRandoms() {
        static_assert(NUMBER_OF_RANDOMS <= TABLE_MEMORY_SIZE);
        if (readIndex + NUMBER_OF_RANDOMS > TABLE_MEMORY_SIZE) {
            readIndex = 0;
        }
        const auto result = randomTable + readIndex;
        readIndex += NUMBER_OF_RANDOMS;
        return result;
    }
};
