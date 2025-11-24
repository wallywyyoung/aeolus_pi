//
// Created by Wally Young on 11/23/25.
//

#pragma once

#include <arm_math.h>
#include <array>
#include <vector>
#include "aeolus/IR.h"

class StereoPartitionedConvolver {
public:
    constexpr static size_t PARTITION_SIZE = PROCESS_FRAMES_SIZE;
    void init(const IR& ir);
    void process(float* left, float* right, const float* firLeft, const float* firRight);
private:
    size_t irSize{ 0 }, numberPartitions{ 0 }, historyIndex{ 0 };
    std::vector<std::array<float, PARTITION_SIZE * 2>> leftInputSpectrumHistory{}, rightInputSpectrumHistory{}, leftIRSpectrum{}, rightIRSpectrum{};
    std::array<float, PARTITION_SIZE * 2> leftOverlap{}, rightOverlap{}, leftBuffer{}, rightBuffer{};
    arm_rfft_fast_instance_f32 leftFFT{}, rightFFT{};
};