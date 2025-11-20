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
#include <arm_neon.h>
#include "RandomTable.h"
#include "aeolus/dsp/Envelope.h"

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
    static_assert(ALSA_BUFFER_SAMPLES_SIZE % 4 == 0);
    const auto min = vdupq_n_f32(-1.0f);
    const auto max = vdupq_n_f32(1.0f);
    const auto scale = vdupq_n_f32(32767.0f);
    for (auto i = 0; i + 4 <= ALSA_BUFFER_SAMPLES_SIZE; i += 4) {
        auto in0 = vld1q_f32(in + i);
        in0 = vmaxq_f32(min, vminq_f32(max, in0));
        in0 = vmulq_f32(in0, scale);
        in0 = vrndaq_f32(in0);
        auto s32 = vcvtq_s32_f32(in0);
        auto s16 = vqmovn_s32(s32);
        s16 = vrshr_n_s16(s16, 3);
        vst1_s16(out + i, s16);
    }
}

static uint8x8x3_t bitpackU32ToS24LE(const uint32x4_t& in0, const uint32x4_t& in1) {
    // Extract the low, middle, and high 8 bits by shifting and narrowing
    const auto lowBytes = vmovn_u16(vcombine_u16(vmovn_u32(in0), vmovn_u32(in1)));
    const auto middleBytes = vmovn_u16(vcombine_u16(vmovn_u32(vshrq_n_u32(in0, 8)), vmovn_u32(vshrq_n_u32(in1, 8))));
    const auto highBytes = vmovn_u16(vcombine_u16(vmovn_u32(vshrq_n_u32(in0, 16)), vmovn_u32(vshrq_n_u32(in1, 16))));
    return {lowBytes, middleBytes, highBytes};
}

static uint32x4_t f32ToS24WordConversion(const float32x4_t& sample) {
    // F32 to S24 scaling factor multiplied times a Volume constant.
    constexpr auto volume = 0.1f;
    const static auto VOLUME_SCALE = vdupq_n_f32(8388607.0f * volume);
    // Cast with hard rounding like vrndaq_f32
    int32x4_t casted = vcvtaq_s32_f32(vmulq_f32(sample, VOLUME_SCALE));
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
        auto in0 = vld1q_f32(in + i);
        auto in1 = vld1q_f32(in + i + 4);
        softClip(in0, in1);
        auto s24W0 = f32ToS24WordConversion(in0);
        auto s24W1 = f32ToS24WordConversion(in1);
        auto packedBytes = bitpackU32ToS24LE(s24W0, s24W1);
        vst3_u8(out + i * 3, packedBytes);
    }
}

void SimdUtilities::copyF32NonInterleavedToInterleaved(const float* left, const float* right, float (&out)[PROCESS_SAMPLES_SIZE]) {
    for (auto i = 0; i + 8 <= PROCESS_FRAMES_SIZE; i += 8) {
        const auto left0 = vld1q_f32(left + i);
        const auto left1 = vld1q_f32(left + i + 4);
        const auto right0 = vld1q_f32(right + i);
        const auto right1 = vld1q_f32(right + i + 4);
        const float32x4x2_t p0 = {left0, right0};
        const float32x4x2_t p1 = {left1, right1};
        vst2q_f32(out + i * 2, p0);
        vst2q_f32(out + i * 2 + 8, p1);
    }
}

void SimdUtilities::lerpF32(const float *a, const float *b, float t, float *out, size_t size) {
    const float32x4_t aT = vdupq_n_f32(1.0f - t);
    const float32x4_t bT = vdupq_n_f32(t);
    auto i = 0;
    for (; i + 8 <= size; i += 8) {
        auto a0 = vld1q_f32(a + i);
        auto a1 = vld1q_f32(a + i + 4);
        auto b0 = vld1q_f32(b + i);
        auto b1 = vld1q_f32(b + i + 4);
        a0 = vmulq_f32(a0, aT);
        a1 = vmulq_f32(a1, aT);
        b0 = vmulq_f32(b0, bT);
        b1 = vmulq_f32(b1, bT);
        vst1q_f32(out + i, vaddq_f32(a0, b0));
        vst1q_f32(out + i + 4, vaddq_f32(a1, b1));
    }
    for (; i < size; ++i) {
        out[i] = a[i] * (1.0f - t) + b[i] * t;
    }
}

// auto index = static_cast<int>(std::floor(delay));
// auto frac = delay - static_cast<float>(index);
// index = (index + writeIndex) % static_cast<int>(circularBuffer.size());
// const auto a = circularBuffer[index];
// const auto b = index < circularBuffer.size() - 1 ? circularBuffer[index + 1] : circularBuffer[0];
// return std::lerp(a, b, frac);

static inline float32x4_t modulo(float32x4_t a, float32_t b) {
    const float32x4_t n = vdupq_n_f32(b);
    return vsubq_f32(a, vmulq_f32(n, vdivq_f32(a, n)));
}


void SimdUtilities::multiplyRandomFactorAdditive(std::array<float, PROCESS_FRAMES_SIZE> &buffer, const float factor) {
    const float32x4_t f = vdupq_n_f32(factor);
    const auto randoms = RandomTable::getInstance().getRandoms<PROCESS_SAMPLES_SIZE>();
    for (auto i = 0; i + 8 <= PROCESS_SAMPLES_SIZE; i += 8) {
        const auto in0 = vld1q_f32(buffer.data() + i);
        const auto in1 = vld1q_f32(buffer.data() + i + 4);
        const auto r0 = vld1q_f32(randoms + i);
        const auto r1 = vld1q_f32(randoms + i + 4);
        const auto m0 = vmulq_f32(in0, vmulq_f32(f, r0));
        const auto m1 = vmulq_f32(in0, vmulq_f32(f, r1));
        const auto o0 = vaddq_f32(in0, m0);
        const auto o1 = vaddq_f32(in1, m1);
        vst1q_f32(buffer.data() + i, o0);
        vst1q_f32(buffer.data() + i + 4, o1);
    }
}

void SimdUtilities::multiplyRandomEnvelopeAdditive(std::array<float, PROCESS_FRAMES_SIZE> &buffer, dsp::Envelope& envelope) {
    const auto randoms = RandomTable::getInstance().getRandoms<PROCESS_SAMPLES_SIZE>();
    for (auto i = 0; i + 8 <= PROCESS_SAMPLES_SIZE; i += 8) {
        const auto in0 = vld1q_f32(buffer.data() + i);
        const auto in1 = vld1q_f32(buffer.data() + i + 4);
        const auto r0 = vld1q_f32(randoms + i);
        const auto r1 = vld1q_f32(randoms + i + 4);
        const float32x4_t e0 = { envelope.next(), envelope.next(), envelope.next(), envelope.next() };
        const float32x4_t e1 = { envelope.next(), envelope.next(), envelope.next(), envelope.next() };
        const auto m0 = vmulq_f32(in0, vmulq_f32(e0, r0));
        const auto m1 = vmulq_f32(in0, vmulq_f32(e1, r1));
        const auto o0 = vaddq_f32(in0, m0);
        const auto o1 = vaddq_f32(in1, m1);
        vst1q_f32(buffer.data() + i, o0);
        vst1q_f32(buffer.data() + i + 4, o1);
    }
}

void SimdUtilities::multiplyFactorAdditive(std::array<float, PROCESS_FRAMES_SIZE> &buffer, std::array<float, PROCESS_FRAMES_SIZE> &swapBuffer, const float factor) {
    const auto f = vdupq_n_f32(factor);
    for (auto i = 0; i + 8 <= PROCESS_SAMPLES_SIZE; i += 8) {
        const auto out0 = vld1q_f32(buffer.data() + i);
        const auto out1 = vld1q_f32(buffer.data() + i + 4);
        const auto in0 = vld1q_f32(swapBuffer.data() + i);
        const auto in1 = vld1q_f32(swapBuffer.data() + i + 4);
        const auto m0 = vmulq_f32(in0, f);
        const auto m1 = vmulq_f32(in0, f);
        const auto o0 = vaddq_f32(out0, m0);
        const auto o1 = vaddq_f32(out1, m1);
        vst1q_f32(buffer.data() + i, o0);
        vst1q_f32(buffer.data() + i + 4, o1);
    }
}
void SimdUtilities::multiplyFactor(float *buffer, const size_t size, const float factor) {
    const auto f = vdupq_n_f32(factor);
    for (auto i = 0; i + 8 <= PROCESS_SAMPLES_SIZE; i += 8) {
        const auto in0 = vld1q_f32(buffer + i);
        const auto in1 = vld1q_f32(buffer + i + 4);
        const auto m0 = vmulq_f32(in0, f);
        const auto m1 = vmulq_f32(in1, f);
        vst1q_f32(buffer + i, m0);
        vst1q_f32(buffer + i + 4, m1);
    }
}

void SimdUtilities::multiplyFactorEnvelopeAdditive(std::array<float, PROCESS_FRAMES_SIZE> &buffer, std::array<float, PROCESS_FRAMES_SIZE> &swapBuffer, const float factor, dsp::Envelope& envelope) {
    const auto f = vdupq_n_f32(factor);
    for (auto i = 0; i + 8 <= PROCESS_SAMPLES_SIZE; i += 8) {
        const auto out0 = vld1q_f32(buffer.data() + i);
        const auto out1 = vld1q_f32(buffer.data() + i + 4);
        const auto in0 = vld1q_f32(swapBuffer.data() + i);
        const auto in1 = vld1q_f32(swapBuffer.data() + i + 4);
        const float32x4_t e0 = { envelope.next(), envelope.next(), envelope.next(), envelope.next() };
        const float32x4_t e1 = { envelope.next(), envelope.next(), envelope.next(), envelope.next() };
        const auto m0 = vmulq_f32(in0, vmulq_f32(f, e0));
        const auto m1 = vmulq_f32(in0, vmulq_f32(f, e1));
        const auto o0 = vaddq_f32(out0, m0);
        const auto o1 = vaddq_f32(out1, m1);
        vst1q_f32(buffer.data() + i, o0);
        vst1q_f32(buffer.data() + i + 4, o1);
    }
}
