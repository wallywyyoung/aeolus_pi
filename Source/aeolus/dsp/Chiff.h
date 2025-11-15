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
#include "aeolus/PipeState.h"
#include "aeolus/StaticPipe.h"
#include "aeolus/dsp/DelayLineStatic.h"
#include "aeolus/dsp/Envelope.h"
#include "aeolus/dsp/Filter.h"

namespace dsp {

/**
 * @brief Pipe wind attack chiff model.
 */
class Chiff {
public:
    Chiff() = default;

    void init(const float& frequency, const float& invertedFrequency, const float& chiffGain, const size_t& surroundingFrames) {
        envelope.init({5.0f * invertedFrequency, 100.0f * invertedFrequency, 0.01f, 100.0f * invertedFrequency});
        pipeDelay = SAMPLE_RATE_F * invertedFrequency;
        lpSpec = {BiquadFilter::LowPass, std::fmin(NYQUIST_WITH_MARGIN * SAMPLE_RATE_F, frequency * 4.0f), BUTTERWORTH_Q, 0.0f};
        chiffDelayFrameCount = static_cast<size_t>(std::min<float>(static_cast<float>(dsp::DelayLineStatic<48000>::size()), 0.5f * invertedFrequency * SAMPLE_RATE_F));
        framesUntilRelease = surroundingFrames + OUTPUT_CHANNELS * chiffDelayFrameCount;
        gain = chiffGain;
        BiquadFilter::updateSpec(lpSpec);
        BiquadFilter::resetState(lpState);
    }

    void process(const PipeState &in, std::array<float, PROCESS_FRAMES_SIZE> &out) {
        if (in.envelopeState == PipeState::OVER) {
            framesUntilRelease -= std::min(framesUntilRelease, static_cast<size_t>(PROCESS_FRAMES_SIZE));
        }
        for (float &sample : out) {
            delayLine.write(sample);
            sample = delayLine.readNearest(chiffDelayFrameCount) * in.outputGain;
        }
        process(out);
    }

    void release() {
        noiseEnvelope.release();
        envelope.release();
    }

    size_t getDelaySampleCount() const {
        return chiffDelayFrameCount;
    }

    [[nodiscard]] bool isActive() const noexcept {
        // Don't care about the noise envelope here
        return envelope.state() != Envelope::Off;
    }

    [[nodiscard]] bool isOver() const noexcept { return framesUntilRelease == 0; }

private:
    constexpr static float BUTTERWORTH_Q = 0.7071f;
    Envelope noiseEnvelope{{0.01f, 0.0f, 1.0f, 0.02f}};
    Envelope envelope;

    DelayLineStatic<SAMPLE_RATE> pipeResonator;
    float pipeDelay{};

    // Feedback low-pass filter;
    BiquadFilter::Spec lpSpec{};
    BiquadFilter::State lpState {};

    DelayLineStatic<SAMPLE_RATE> delayLine{};
    size_t chiffDelayFrameCount{};
    size_t framesUntilRelease{};  ///< Counter to account for the delayed sound before recycling the voice.

    float gain{};

    void process(std::array<float, PROCESS_FRAMES_SIZE> &out);
};

} // namespace dsp


