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

#include "aeolus/dsp/SpatialSource.h"
#include <arm_math.h>
#include <array>
#include <cmath>
#include "MemoryConstants.h"
#include "aeolus/utilities/SimdUtilities.h"

void SpatialSource::Position::rotate(const float a) {
    const float c = arm_cos_f32(a);
    const float s = arm_sin_f32(a);
    const float x2 = c * x - s * y;
    const float y2 = s * x + s * y;
    x = x2;
    y = y2;
}

float SpatialSource::Position::distanceTo(const Position &other) const noexcept {
    float out{};
    arm_sqrt_f32((other.x - x) * (other.x - x) + (other.y - y) * (other.y - y), &out);
    return out;
}

float SpatialSource::Position::angleTo(const Position &other) const noexcept {
    float out1{}, out2{};
    arm_atan2_f32(other.y, other.x, &out1);
    arm_atan2_f32(y, x, &out2);
    return out1 - out2;
}

void SpatialSource::init(const int note, const float fd, const float fn) {
    sourcePosition.x = STARTING_STEREO_WIDTH * fd / fn * (note % 2 != 0 ? 1.0f : -1.0f) * static_cast<float>(abs(note - MIDDLE_C));
    sourcePosition.y = PIPE_HEIGHT;
    recalculate();
}

void SpatialSource::reset() {
    delayLine.reset();
    lowPassFilterL.reset();
    lowPassFilterR.reset();
}

void SpatialSource::process(const std::array<float, PROCESS_FRAMES_SIZE> &in, StaticAudioBuffer<PROCESS_FRAMES_SIZE, OUTPUT_CHANNELS> &out) {
    auto l = out.getWritePointer(0);
    auto r = out.getWritePointer(1);
    delayLine.process(in, l, r, leftDelay, rightDelay);
    lowPassFilterL.process(l, l);
    lowPassFilterR.process(r, r);
    SimdUtilities::multiplyFactor(l, PROCESS_FRAMES_SIZE, leftAttenuation);
    SimdUtilities::multiplyFactor(r, PROCESS_FRAMES_SIZE, rightAttenuation);
}

static float distanceToCutOffFrequency(const float d) {
    const float in{-0.09f * d};
    float out{};
    arm_vexp_f32(&in,&out,1);
    return 22.0e3f * out;
}

void SpatialSource::recalculate() {
    static constexpr auto SPEED_OF_SOUND_R = 1.0f / 343.0f; // [m/s] @ 20 deg C

    Position left{-0.5f * listenerLeftRightDistance, 0.0f};
    Position right{0.5f * listenerLeftRightDistance, 0.0f};
    left.rotate(listenerOrientation);
    right.rotate(listenerOrientation);

    const Position sourceRelativeToListener{sourcePosition.x - listenerPosition.x, sourcePosition.y - listenerPosition.y};
    const float leftAngle = left.angleTo(sourceRelativeToListener);
    const float rightAngle = right.angleTo(sourceRelativeToListener);

    left.x += listenerPosition.x;
    left.y += listenerPosition.y;
    right.x += listenerPosition.x;
    right.y += listenerPosition.y;

    const auto leftDistance = sourcePosition.distanceTo(left);
    const auto rightDistance = sourcePosition.distanceTo(right);
    const auto maxDistance = std::max(leftDistance, rightDistance);
    const auto maxT = maxDistance * SPEED_OF_SOUND_R;
    const auto delayLengthInSamples = static_cast<size_t>(std::lround(SAMPLE_RATE_F * maxT));
    delayLine.resize(delayLengthInSamples + PROCESS_FRAMES_SIZE);

    leftDelay = static_cast<int>(roundf(leftDistance * SAMPLE_RATE_F * SPEED_OF_SOUND_R));
    rightDelay = static_cast<int>(roundf(rightDistance * SAMPLE_RATE_F * SPEED_OF_SOUND_R));

    constexpr auto att = 0.7f; // [0..1]

    // Angular attenuation
    leftAttenuation = 0.5f * att * (arm_cos_f32(leftAngle) + 1.0f) + 1.0f - att;
    rightAttenuation = 0.5f * att * (arm_cos_f32(rightAngle) + 1.0f) + 1.0f - att;

    lowPassFilterL.calculateCoefficients(distanceToCutOffFrequency(leftDistance));
    lowPassFilterR.calculateCoefficients(distanceToCutOffFrequency(rightDistance));
}

[[nodiscard]] size_t SpatialSource::getPostFxSamplesCount() const {
    return delayLine.size();
}