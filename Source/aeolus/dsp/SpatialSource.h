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

#include "LowPassFilter.h"
#include "StaticAudioBuffer.h"
#include "aeolus/dsp/DelayLine.h"

/**
 * @brief Sound source spatial modeller.
 *
 * This class take mono audio source and models a stereo output
 * based on the source and listener relative positions.
 * All positioning is performed in 2D space. Positions are specified in meters.
 */
class SpatialSource {
public:
    struct Position {
        float x;
        float y;
        void rotate(float a);
        [[nodiscard]] float distanceTo(const Position &other) const noexcept;
        [[nodiscard]] float angleTo(const Position &other) const noexcept;
    };

    SpatialSource() = default;

    void init(int note, float fd, float fn);
    void reset();
    void process(const std::array<float, PROCESS_FRAMES_SIZE> &in, StaticAudioBuffer<PROCESS_FRAMES_SIZE, OUTPUT_CHANNELS> &out);
    void recalculate();
    [[nodiscard]] size_t getPostFxSamplesCount() const;

private:
    constexpr static auto STARTING_STEREO_WIDTH = 0.15f;
    constexpr static auto MIDDLE_C = 65;
    constexpr static auto PIPE_HEIGHT = 5.0f;

    Position sourcePosition;
    Position listenerPosition{0.0f, 0.0f};
    float listenerOrientation{0.0f};
    float listenerLeftRightDistance{0.3f};

    DelayLine delayLine{};
    int leftDelay{};
    int rightDelay{};
    float leftAttenuation{};
    float rightAttenuation{};

    LowPassFilter lowPassFilterL{};
    LowPassFilter lowPassFilterR{};
};
