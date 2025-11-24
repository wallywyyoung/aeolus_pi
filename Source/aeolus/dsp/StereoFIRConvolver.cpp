//
// Created by Wally Young on 11/23/25.
//

#include "StereoFIRConvolver.h"
#include <algorithm>

void StereoFIRConvolver::init(const IR& ir) {
    const auto left = ir.getReadPointer(0);
    const auto right = ir.getReadPointer(1);
    std::reverse_copy(left, left + NUMBER_TAPS, leftCoefficients.begin());
    std::reverse_copy(right, right + NUMBER_TAPS, rightCoefficients.begin());
    arm_fir_init_f32(&leftInstance, NUMBER_TAPS, leftCoefficients.data(), leftState.data(), PROCESS_FRAMES_SIZE);
    arm_fir_init_f32(&rightInstance, NUMBER_TAPS, rightCoefficients.data(), rightState.data(), PROCESS_FRAMES_SIZE);
}

void StereoFIRConvolver::process(const float *inLeft, const float *inRight, float *outLeft, float *outRight) const {
    arm_fir_f32(&leftInstance, inLeft, outLeft, PROCESS_FRAMES_SIZE);
    arm_fir_f32(&rightInstance, inRight, outRight, PROCESS_FRAMES_SIZE);
}