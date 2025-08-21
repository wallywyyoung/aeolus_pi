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
constexpr static Channels OUTPUT_CHANNELS = Stereo;
constexpr static Channels VOICE_CHANNELS = Stereo;
constexpr static SampleRate SAMPLE_RATE = kHz48000;
constexpr static float SAMPLE_RATE_F = static_cast<float>(SAMPLE_RATE);
constexpr static float SAMPLE_RATE_R = 1.0f / SAMPLE_RATE_F;
constexpr static int PERIOD_SIZE = static_cast<int>(SAMPLE_RATE_F * 0.01f);
constexpr static int NUMBER_FRAMES = PERIOD_SIZE * 2;
constexpr static int NUMBER_SAMPLES = NUMBER_FRAMES * OUTPUT_CHANNELS;

constexpr static int AUDIO_SUB_FRAME_LENGTH = 512; //PERIOD_SIZE; // Length of a processing frame (in samples).
static constexpr unsigned short CACHE_LINE_SIZE = 64; // for Raspberry Pi 4B

template <typename T>
constexpr bool isPowerOfTwo(T n) {
    return (n > 0) && ((n & (n - 1)) == 0);
}
static_assert(isPowerOfTwo(AUDIO_SUB_FRAME_LENGTH), "AUDIO_SUB_FRAME_LENGTH must be a power of two.");;