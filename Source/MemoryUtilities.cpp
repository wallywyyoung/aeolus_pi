//
// Created by Wally Young on 8/3/25.
//

#include "MemoryUtilities.h"

#include <arm_neon.h>
#include <cassert>
#include <cmath>
#include <algorithm>

void MemoryUtilities::enableFlushToZero() {
    uint64_t fpsr;
    constexpr uint64_t ftz = 1UL << 24;
    asm volatile("mrs %0, fpcr" : "=r"(fpsr));
    asm volatile("msr fpcr, %0" : : "ri"(fpsr | ftz));
}

void MemoryUtilities::disableFlushToZero() {
    uint64_t fpsr;
    constexpr uint64_t ftz = 1UL << 24;
    asm volatile("mrs %0, fpcr" : "=r"(fpsr));
    asm volatile("msr fpcr, %0" : : "ri"(fpsr & ~ftz));
}

void MemoryUtilities::ConvertF32toS16(float (&in)[NUMBER_SAMPLES], int16_t* out) {
    const float32x4_t min = vdupq_n_f32(-1.0f);
    const float32x4_t max = vdupq_n_f32(1.0f);
    const float32x4_t volume = vdupq_n_f32(0.005f);
    const float32x4_t scale = vdupq_n_f32(32767.0f);

    int i = 0;
    for (; i + 4 <= NUMBER_SAMPLES; i += 4) {
        float32x4_t in0 = vld1q_f32(in + i);
        float32x4_t clamped0 = vmaxq_f32(min, vminq_f32(max, in0));
        float32x4_t scaled0 = vmulq_f32(clamped0, scale);
        float32x4_t rounded0 = vrndaq_f32(scaled0);
        int32x4_t s32 = vcvtq_s32_f32(rounded0);
        int16x4_t s16 = vqmovn_s32(s32);
        int16x4_t attenuated = vrshr_n_s16(s16, 3);
        vst1_s16(out + i, attenuated);
    }
    for (; i < NUMBER_SAMPLES; ++i) {
        out[i] = std::round(32767.0f * std::clamp(in[i], -1.0f, 1.0f));
    }
}

void MemoryUtilities::ConvertF32toS24(float(&in)[NUMBER_SAMPLES], uint8_t* out) {
    const float32x4_t min = vdupq_n_f32(-1.0f);
    const float32x4_t max = vdupq_n_f32(1.0f);
    const float32x4_t volume = vdupq_n_f32(0.005f);
    const float32x4_t scale = vdupq_n_f32(8388607.0f);

    int i = 0;
    for (; i + 4 <= NUMBER_SAMPLES; i += 4) {
        float32x4_t in0 = vld1q_f32(in + i);
        float32x4_t clamped0 = vmaxq_f32(min, vminq_f32(max, in0));
        float32x4_t scaled0 = vmulq_f32(clamped0, scale);
        float32x4_t rounded0 = vrndaq_f32(scaled0);
        int32x4_t s32 = vcvtq_s32_f32(rounded0);
        int32x4_t attenuated = vrshrq_n_s32(s32, 3);

        // Bit packing.
        uint32_t value = vgetq_lane_s32(attenuated, 0);
        out[i * 3 + 0] = value & 0xFF;
        out[i * 3 + 1] = value >> 8 & 0xFF;
        out[i * 3 + 2] = value >> 16 & 0xFF;
        value = vgetq_lane_s32(attenuated, 1);
        out[i * 3 + 3] = value & 0xFF;
        out[i * 3 + 4] = value >> 8 & 0xFF;
        out[i * 3 + 5] = value >> 16 & 0xFF;
        value = vgetq_lane_s32(attenuated, 2);
        out[i * 3 + 6] = value & 0xFF;
        out[i * 3 + 7] = value >> 8 & 0xFF;
        out[i * 3 + 8] = value >> 16 & 0xFF;
        value = vgetq_lane_s32(attenuated, 3);
        out[i * 3 + 9] = value & 0xFF;
        out[i * 3 + 10] = value >> 8 & 0xFF;
        out[i * 3 + 11] = value >> 16 & 0xFF;
    }
    for (; i < NUMBER_SAMPLES; ++i) {
        const auto val = static_cast<int32_t>(std::round(8388607.0f * std::clamp(in[i], -1.0f, 1.0f)));
        out[i * 3 + 0] = val & 0xFF;
        out[i * 3 + 1] = val >> 8 & 0xFF;
        out[i * 3 + 2] = val >> 16 & 0xFF;
    }
}
