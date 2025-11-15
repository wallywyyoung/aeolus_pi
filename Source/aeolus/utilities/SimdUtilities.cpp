// ----------------------------------------------------------------------------
//
//  Copyright (C) 2025 Wally Young <wallywyyoung@users.noreply.github.com>
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 3 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
// ----------------------------------------------------------------------------

#include "aeolus/utilities/SimdUtilities.h"

#include <algorithm>
#include <arm_neon.h>
#include <cassert>
#include <cmath>
#include <iostream>
#include <ostream>
#include <stdint.h>

void SimdUtilities::enableFlushToZero() {
    uint64_t fpsr;
    constexpr uint64_t ftz = 1UL << 24;
    asm volatile("mrs %0, fpcr" : "=r"(fpsr));
    asm volatile("msr fpcr, %0" : : "ri"(fpsr | ftz));
}

void SimdUtilities::disableFlushToZero() {
    uint64_t fpsr;
    constexpr uint64_t ftz = 1UL << 24;
    asm volatile("mrs %0, fpcr" : "=r"(fpsr));
    asm volatile("msr fpcr, %0" : : "ri"(fpsr & ~ftz));
}

void SimdUtilities::convertF32ToS16(const float (&in)[ALSA_BUFFER_SAMPLES_SIZE], std::int16_t* out) {
    const float32x4_t min = vdupq_n_f32(-1.0f);
    const float32x4_t max = vdupq_n_f32(1.0f);
    const float32x4_t scale = vdupq_n_f32(32767.0f);

    auto i = 0;
    for (; i + 4 <= ALSA_BUFFER_SAMPLES_SIZE; i += 4) {
        float32x4_t in0 = vld1q_f32(in + i);
        float32x4_t clamped0 = vmaxq_f32(min, vminq_f32(max, in0));
        float32x4_t scaled0 = vmulq_f32(clamped0, scale);
        float32x4_t rounded0 = vrndaq_f32(scaled0);
        int32x4_t s32 = vcvtq_s32_f32(rounded0);
        int16x4_t s16 = vqmovn_s32(s32);
        int16x4_t attenuated = vrshr_n_s16(s16, 3);
        vst1_s16(out + i, attenuated);
    }
    for (; i < ALSA_BUFFER_SAMPLES_SIZE; ++i) {
        out[i] = std::round(32767.0f * std::clamp(in[i], -1.0f, 1.0f));
    }
}

static uint8x8x3_t bitpackU32ToS24LE(const uint32x4_t& in0, const uint32x4_t& in1) {
    // Extract the middle 8 bits by shifting and narrowing
    uint8x8_t lowBytes = vmovn_u16(vcombine_u16(vmovn_u32(in0), vmovn_u32(in1)));
    // Extract the middle 8 bits by shifting and narrowing
    uint8x8_t midBytes = vmovn_u16(vcombine_u16(vmovn_u32(vshrq_n_u32(in0, 8)), vmovn_u32(vshrq_n_u32(in1, 8))));
    // Extract the high 8 bits by shifting and narrowing
    uint8x8_t highBytes = vmovn_u16(vcombine_u16(vmovn_u32(vshrq_n_u32(in0, 16)), vmovn_u32(vshrq_n_u32(in1, 16))));
    // Interleave the data
    return {lowBytes, midBytes, highBytes};
}

static uint32x4_t f32ToS24WordConversion(const float32x4_t& sample) {
    // F32 to S24 scaling factor multiplied times a Volume constant.
    const auto volumeScale = vdupq_n_f32(8388607.0f * 0.03);
    // Cast with hard rounding like vrndaq_f32
    int32x4_t casted = vcvtaq_s32_f32(vmulq_f32(sample, volumeScale));
    // uint32x4_t required for bit packing intrinsics
    return vreinterpretq_u32_s32(casted);
}

static void softClip(float32x4_t& a, float32x4_t& b) {
    // x * (27 + x^2) / (27 + 9 * x^2)
    auto xSquared0 = vmulq_f32(a, a);
    auto xSquared1 = vmulq_f32(b, b);

    const auto c27 = vdupq_n_f32(27.0f);
    auto numerator0 = vmulq_f32(vaddq_f32(xSquared0, c27), a);
    auto numerator1 = vmulq_f32(vaddq_f32(xSquared1, c27), b);

    const auto c9 = vdupq_n_f32(9.0f);
    auto denominator0 = vaddq_f32(vmulq_f32(xSquared0, c9), c27);
    auto denominator1 = vaddq_f32(vmulq_f32(xSquared1, c9), c27);

    auto rcp0 = vrecpeq_f32(denominator0);
    auto rcp1 = vrecpeq_f32(denominator1);
    rcp0 = vmulq_f32(vrecpsq_f32(denominator0, rcp0), rcp0);
    rcp1 = vmulq_f32(vrecpsq_f32(denominator1, rcp1), rcp1);

    a = vmulq_f32(numerator0, rcp0);
    b = vmulq_f32(numerator1, rcp1);
}

void SimdUtilities::convertF32ToS24(const float (&in)[PROCESS_SAMPLES_SIZE], std::uint8_t(&out)[PROCESS_SAMPLES_SIZE * 3]) {
    for (auto i = 0; i + 8 <= PROCESS_SAMPLES_SIZE; i += 8) {
        // Load
        auto in0 = vld1q_f32(in + i);
        auto in1 = vld1q_f32(in + i + 4);

        // Soft Clip
        softClip(in0, in1);

        // Convert
        in0 = f32ToS24WordConversion(in0);
        in1 = f32ToS24WordConversion(in1);

        // Pack
        auto packedBytes = bitpackU32ToS24LE(in0, in1);

        // Write
        vst3_u8(out + i * 3, packedBytes);
    }
}
