// ----------------------------------------------------------------------------
//
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

namespace dsp {

/**
 * @brief ADSR-style envelope with exponential slopes.
 */
class Envelope {
public:
    enum State { Off = 0, Attack, Decay, Sustain, Release, NumStates };
    constexpr static auto ATTACK_TARGET_RATIO = 0.3f;
    constexpr static auto DECAY_RELEASE_TARGET_RATIO = 0.0001f;
    struct Trigger { float attack = 0.0f; float decay = 0.0f; float sustain = 1.0f; float release = 1.0f; };

    explicit Envelope(const Trigger& trigger);

    [[nodiscard]] State state() const noexcept { return currentState; }
    void release();
    void release(float t);

    float next();

    [[nodiscard]] float level() const noexcept { return currentLevel; }

private:

    static float calculate(float rate, float targetRatio);

    State currentState{ Attack };
    float currentLevel{ 0.0f };

    float attackRate;
    float attackCoefficient;
    float attackBase;

    float decayRate;
    float decayCoefficient;
    float decayBase;

    float releaseRate;
    float releaseCoefficient;
    float releaseBase;

    float sustainLevel;
};

} // namespace dsp


