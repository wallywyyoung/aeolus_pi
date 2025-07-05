// ----------------------------------------------------------------------------
//
//  Copyright (C) 2025 Wally Young <wallywyyoung@users.noreply.github.com>
//  Copyright (C) 2021 Arthur Benilov <arthur.benilov@gmail.com>
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

#include <cmath>

template <typename T> T limitRange(T min, T max, T value) {
    return std::max(min, std::min(max, value));
}

// TODO: Fix mixed comparison with large unsigned types.
template <typename T1, typename T2> bool isPositiveAndBelow(T1 instance, T2 threshold) {
    return T1() <= instance && instance < static_cast<T1>(threshold);
}

/// Multibus output option (must be set in the project configuration)
#ifndef AEOLUS_MULTIBUS_OUTPUT
#   define AEOLUS_MULTIBUS_OUTPUT 0
#endif

#if AEOLUS_MULTIBUS_OUTPUT
    constexpr static int N_OUTPUT_CHANNELS = 8;
    constexpr static int N_VOICE_CHANNELS = 1;
#else
    constexpr static int N_OUTPUT_CHANNELS = 2;
    constexpr static int N_VOICE_CHANNELS = 2;
#endif

/// Processing sample rate. It is low enough
/// since there are not many harmonics to be generated
/// and thus we can get away without using an interpolation filter
/// when upsampling only.
constexpr static int SAMPLE_RATE = 44100;
constexpr static float SAMPLE_RATE_F = (float) SAMPLE_RATE;
constexpr static float SAMPLE_RATE_R = 1.0f / SAMPLE_RATE_F;
constexpr static size_t BPS_RATE = SAMPLE_RATE * 2 /* 16-bit */ * N_OUTPUT_CHANNELS;

/// Length of a processing frame (in samples).
constexpr static int SUB_FRAME_LENGTH = 64;

// MIDI controls
enum {
    CC_MODULATION = 1,
    CC_VOLUME = 7,
    CC_REVERB = 91,
    CC_STOP_BUTTONS = 98,
    CC_ALL_NOTES_OFF = 123
};

namespace math {

float exp2ap(float x);

/// Linear interpolation
template <typename T>
T lerp (T a, T b, T frac) { return a + (b - a) * frac; }

template <typename T>
T lagr (const T* const x, T frac) noexcept
{
    const T c1 = x[2] - (1.0f / 3.0f) * x[0] - 0.5f * x[1] - (1.0f / 6.0f) * x[3];
    const T c2 = 0.5f * (x[0] + x[2]) - x[1];
    const T c3 = (1.0f / 6.0f) * (x[3] - x[0]) + 0.5f * (x[1] - x[2]);
    return ((c3 * frac + c2) * frac + c1) * frac + x[1];
}

template<unsigned M, unsigned N, unsigned B, unsigned A>
struct SinCosSeries
{
    constexpr static double value =
        1.0 - (A * M_PI/ B) * ( A * M_PI / B) / M / (M + 1)
        * SinCosSeries<M + 2, N, B, A>::value;
};

template<unsigned N, unsigned B, unsigned A>
struct SinCosSeries<N, N, B, A> {
    constexpr static double value = 1.0;
};

template<unsigned B, unsigned A, typename T = double>
struct Sin;

template<unsigned B, unsigned A>
struct Sin<B, A, float>
{
    constexpr static float value = (A * static_cast<float>(M_PI) / B) * float (SinCosSeries<2, 24, B, A>::value);
};

template<unsigned B, unsigned A>
struct Sin<B, A, double> {
    constexpr static double value = (A * static_cast<float>(M_PI) / B) * SinCosSeries<2, 34, B, A>::value;
};

template <typename T>
constexpr bool isPowerOfTwo(T v)
{
    return (v & (v - 1)) == 0;
}

} // namespace math

//----------------------------------------------------------

namespace midi {
    int midiChannelToMask(int channel);
    bool matchMidiChannelToMask(int mask, int channel);
} // namespace midi


