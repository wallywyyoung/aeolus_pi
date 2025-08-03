//
// Created by Wally Young on 8/3/25.
//

#include "MemoryUtilities.h"
#include <arm_neon.h>
#include <cassert>
#include <stdexcept>

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

void MemoryUtilities::ConvertF32toS16(float (&in)[AUDIO_BUFFER_SIZE], int16_t* out) {
    const float32x4_t min = vdupq_n_f32(-1.0f);
    const float32x4_t max = vdupq_n_f32(1.0f);
    const float32x4_t scale = vdupq_n_f32(32767.0f);

    int i = 0;
    for (; i + 4 <= AUDIO_BUFFER_SIZE; i += 4) {
        float32x4_t in0 = vld1q_f32(in + i);
        float32x4_t clamped = vmaxq_f32(min, vminq_f32(max, in0));
        float32x4_t scaled = vmulq_f32(clamped, scale);
        float32x4_t rounded = vrndaq_f32(scaled);
        int32x4_t s32 = vcvtq_s32_f32(rounded);
        int16x4_t s16 = vqmovn_s32(s32);
        vst1_s16(out + i, s16);
    }
}