//
// Created by Wally Young on 11/15/25.
//

#pragma once

#include <arm_math.h>

class LowPassFilter {
    arm_biquad_cascade_df2T_instance_f32 biquadInstance{};
    float32_t coefficients[5]{ };
    float32_t state[2] { 0.0f, 0.0f};

public:
    void reset();
    void calculateCoefficients(float cutoffFrequency);
    void process(const float32_t* in, float* out);
};

