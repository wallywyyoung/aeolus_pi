//
// Created by Wally Young on 11/15/25.
//

#include "LowPassFilter.h"
#include <algorithm>
#include <numbers>
#include "MemoryConstants.h"

void LowPassFilter::reset() {
    std::fill_n(state, 2, 0.0f);
}

void LowPassFilter::calculateCoefficients(const float cutoffFrequency) {
    constexpr auto q2r = 1.0f / (0.7073f * 2.0f);
    const auto w0 = 2.0f * std::numbers::pi_v<float> * cutoffFrequency * SAMPLE_RATE_R;
    const auto alpha = arm_sin_f32(w0) * q2r;
    const auto cosW0 = arm_cos_f32(w0);
    coefficients[0] = ( 1.0f - cosW0) * 0.5f; // b0
    coefficients[1] = 1.0f - cosW0; // b1
    coefficients[2] = coefficients[0]; // b2
    coefficients[3] = -1.0f * (-2.0f * cosW0); // a1
    coefficients[4] = -1.0f * (1.0f - alpha); // a2
    const auto a0r = 1.0f / (1.0f + alpha); // a0
    for (auto& coefficient : coefficients) {
        coefficient *= a0r;
    }
    arm_biquad_cascade_df2T_init_f32(&biquadInstance, 1, coefficients, state);
}

void LowPassFilter::process(const float32_t* in, float* out) {
    arm_biquad_cascade_df2T_f32(&biquadInstance, in, out, PROCESS_FRAMES_SIZE);
}