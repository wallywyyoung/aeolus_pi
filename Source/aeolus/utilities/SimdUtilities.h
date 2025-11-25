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

#pragma once

#include <array>
#include <cstdint>
#include "MemoryConstants.h"

namespace dsp {
    class Envelope;
}
class SimdUtilities {
public:
    // Using inline assembly for ARMv8-a enabling flush-to-zero
    // Denormals are handled differently in ARMv8-a so there is no equivalent denormals-are-zero
    // Ensure your in and out buffers are aligned to 16 to be NEON compliant.
    static void enableFlushToZero();
    static void disableFlushToZero();
    static void convertF32ToS16(const float (&in)[ALSA_BUFFER_SAMPLES_SIZE], std::int16_t * out);
    static void convertF32ToS24(const float (&in)[PROCESS_SAMPLES_SIZE], std::uint8_t(&out)[PROCESS_SAMPLES_SIZE * 3]);
    static void convertF32NonInterleavedToS24Interleaved(const float (&input)[PROCESS_SAMPLES_SIZE], std::uint8_t (&output)[PROCESS_SAMPLES_SIZE * 3]);
    static void copyF32NonInterleavedToInterleaved(const float* left, const float* right, float (&out)[PROCESS_SAMPLES_SIZE]);
    static void lerpF32(const float *a, const float *b, float t, float *out, size_t size);
    static void reverseLerpF32(const float *a, const float *b, float t, float *out, size_t size);
    static void multiplyRandomFactorAdditive(std::array<float, PROCESS_FRAMES_SIZE> &buffer, float factor);
    static void multiplyRandomEnvelopeAdditive(std::array<float, PROCESS_FRAMES_SIZE> &buffer, dsp::Envelope& envelope);
    static void multiplyFactorAdditive(std::array<float, PROCESS_FRAMES_SIZE> &buffer, std::array<float, PROCESS_FRAMES_SIZE> &swapBuffer, float factor);
    static void multiplyFactor(float* buffer, size_t size, float factor);
    static void multiplyFactorEnvelopeAdditive(std::array<float, PROCESS_FRAMES_SIZE> &buffer, std::array<float, PROCESS_FRAMES_SIZE> &swapBuffer, float factor, dsp::Envelope& envelope);
    static void add(float* to, const float* from, size_t size);
private:
    // Ensure your in and out buffers are aligned to 16 to be NEON compliant.
    static float maxF32(const float (&in)[PROCESS_SAMPLES_SIZE]);
};
