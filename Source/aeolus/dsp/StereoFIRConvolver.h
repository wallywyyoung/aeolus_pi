//
// Created by Wally Young on 11/23/25.
//

#pragma once

#include <arm_math.h>
#include <array>
#include "MemoryConstants.h"
#include "aeolus/IR.h"

class StereoFIRConvolver {
public:
    static constexpr size_t NUMBER_TAPS = PROCESS_FRAMES_SIZE;
    void init(const IR& ir);
    void process(const float *inLeft, const float *inRight, float *outLeft, float *outRight) const;
private:
    std::array<float, NUMBER_TAPS> leftCoefficients{}, rightCoefficients{};
    std::array<float, NUMBER_TAPS + PROCESS_FRAMES_SIZE - 1> leftState{}, rightState{};
    arm_fir_instance_f32 leftInstance{}, rightInstance{};
};
