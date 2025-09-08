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

/// Processing sample rate.
/// There are few harmonics generated, so can be set this low.
/// When upsampling, we can get away without using an interpolation filter.
enum SampleRate {
    kHz44100 = 44100,
    kHz48000 = 48000
};
enum Channels {
    Stereo = 2
};

// constexpr int nearestPowerOfTwo(float n) {
//     int last = 0;
//     for (auto i = 1; i < 100; ++i) {
//         auto now = static_cast<int>(std::pow(2, i));
//         if (last < n && n <= now) {
//             return now;
//         }
//         last = now;
//     }
// }

constexpr static Channels OUTPUT_CHANNELS = Stereo;                     ///< Number of channels in output buffer.
constexpr static SampleRate SAMPLE_RATE = kHz48000;                     ///< The sample rate.
constexpr static float SAMPLE_RATE_F = static_cast<float>(SAMPLE_RATE); ///< Float of sample rate.
constexpr static float SAMPLE_RATE_R = 1.0f / SAMPLE_RATE_F;            ///< Float inversion of sample rate.
constexpr static int PROCESS_FRAMES_SIZE = 256;                     ///< Number of frames batch processed. PoT.
constexpr static int PROCESS_SAMPLES_SIZE = PROCESS_FRAMES_SIZE * 2;                     ///< Number of samples batch processed. PoT.
constexpr static int ALSA_PERIOD_SIZE = PROCESS_FRAMES_SIZE;        ///< ALSA period size. PoT.
constexpr static int ALSA_MINIMUM_FRAMES = PROCESS_FRAMES_SIZE;     ///< Minimum number of frames to be available from ALSA.
constexpr static int ALSA_THRESHOLD = ALSA_PERIOD_SIZE * 2;             ///< Number of frames before ALSA begins playback.
constexpr static int ALSA_BUFFER_FRAMES_SIZE = ALSA_PERIOD_SIZE * 4;              ///< Number of frames in output buffer.
constexpr static int ALSA_BUFFER_SAMPLES_SIZE = ALSA_BUFFER_FRAMES_SIZE * OUTPUT_CHANNELS;  ///< Number of samples in output buffer.
static constexpr unsigned short CACHE_LINE_SIZE = 64;                   ///< Cache alignment for Raspberry Pi 4B

static constexpr auto NYQUIST_WITH_MARGIN = 5.0f - 0.05f;
template <typename T>
constexpr bool isPowerOfTwo(T n) {
    return (n > 0) && ((n & (n - 1)) == 0);
}

static_assert(isPowerOfTwo(PROCESS_FRAMES_SIZE), "PROCESS_FRAMES_SIZE must be a power of two.");;