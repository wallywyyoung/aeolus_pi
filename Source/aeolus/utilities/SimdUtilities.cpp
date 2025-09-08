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
#include <stdint.h>
#include <arm_neon.h>
#include <cassert>
#include <cmath>

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

void SimdUtilities::ConvertF32toS16(const float (&in)[ALSA_BUFFER_SAMPLES_SIZE], std::int16_t* out) {
    const float32x4_t min = vdupq_n_f32(-1.0f);
    const float32x4_t max = vdupq_n_f32(1.0f);
    const float32x4_t scale = vdupq_n_f32(32767.0f);

    int i = 0;
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

void SimdUtilities::ConvertF32toS24(const float (&in)[PROCESS_SAMPLES_SIZE], std::uint8_t(&out)[PROCESS_SAMPLES_SIZE * 3]) {
    const float32x4_t min = vdupq_n_f32(-1.0f);
    const float32x4_t max = vdupq_n_f32(1.0f);
    const float32x4_t scale = vdupq_n_f32(8388607.0f);

    for (auto i = 0; i + 4 <= ALSA_BUFFER_SAMPLES_SIZE; i += 8) {
        // Convert f32 into s24
        float32x4_t in0 = vld1q_f32(in + i);                                 // Input
        float32x4_t clamped0 = vmaxq_f32(min, vminq_f32(max, in0));  // Clamp
        float32x4_t scaled0 = vmulq_f32(clamped0, scale);                 // Scale
        float32x4_t rounded0 = vrndaq_f32(scaled0);                            // Round
        int32x4_t attenuated0 = vcvtq_s32_f32(rounded0);                               // Cast
        // int32x4_t  = vrshrq_n_s32(s320, 3);                        // Attenuate
        float32x4_t in1 = vld1q_f32(in + i + 4);                                 // Input
        float32x4_t clamped1 = vmaxq_f32(min, vminq_f32(max, in1));  // Clamp
        float32x4_t scaled1 = vmulq_f32(clamped1, scale);                 // Scale
        float32x4_t rounded1 = vrndaq_f32(scaled1);                            // Round
        int32x4_t attenuated1 = vcvtq_s32_f32(rounded1);                               // Cast
        // int32x4_t attenuated1 = vrshrq_n_s32(s321, 3);                        // Attenuate

        // Bitpack s24 into the output array
        uint32x4_t u320 = vreinterpretq_u32_s32(attenuated0); // uint32x4_t required for bit packing intrinsics
        uint32x4_t u321 = vreinterpretq_u32_s32(attenuated1); // uint32x4_t required for bit packing intrinsics


        uint16x4_t lowBits0 = vmovn_u32(u320); // Extract the lower 8 bits
        uint16x4_t lowBits1 = vmovn_u32(u321); // Extract the lower 8 bits
        uint8x8_t lowBytes = vmovn_u16(vcombine_u16(lowBits0, lowBits1)); // Store the lower 8 bits together

        // Extract the middle 8 bits by shifting and narrowing
        uint8x8_t midBytes = vmovn_u16(vcombine_u16(vmovn_u32(vrshrq_n_u32(u320, 8)), vmovn_u32(vrshrq_n_u32(u321, 8))));

        // Extract the high 8 bits by shifting and narrowing
        uint8x8_t highBytes = vmovn_u16(vcombine_u16(vmovn_u32(vrshrq_n_u32(u320, 16)), vmovn_u32(vrshrq_n_u32(u321, 16))));

        uint8x8x3_t packedBytes = {lowBytes, midBytes, highBytes}; // Interleave the data
        vst3_u8(out + i * 3, packedBytes); // Write interleaved data all at once
    }
}
