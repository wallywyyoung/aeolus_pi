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

#include "MemoryConstants.h"
#include "aeolus/dsp/DelayLineStatic.h"
#include "aeolus/dsp/Envelope.h"
#include "aeolus/dsp/filter.h"

namespace dsp {

/**
 * @brief Pipe wind attack chiff model.
 */
class Chiff {
    //TODO : SEE IF CHIFF IS BROKEN. DELAY LINE RELIES ON CHIFF
public:
    Chiff(const float& frequency, const float& invertedFrequency, const float& chiffGain);

    void release();

    [[nodiscard]] bool isActive() const noexcept;

    void process(std::array<float, PROCESS_FRAMES_SIZE> &out);

private:
    constexpr static float BUTTERWORTH_Q = 0.7071f;
    Envelope noiseEnvelope{{0.01f, 0.0f, 1.0f, 0.02f}};
    Envelope envelope;

    DelayLineStatic<SAMPLE_RATE> pipeResonator;
    float pipeDelay;

    // Feedback low-pass filter;
    BiquadFilter::Spec lpSpec;
    BiquadFilter::State lpState {};

    float gain;
};

} // namespace dsp


