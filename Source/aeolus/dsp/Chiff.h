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
#include "MemoryConstants.h"
#include "aeolus/PipeState.h"
#include "aeolus/StaticPipe.h"
#include "aeolus/dsp/DelayLineStatic.h"
#include "aeolus/dsp/Envelope.h"

/**
 * @brief Pipe wind attack chiff model.
 */
class Chiff {
public:
    Chiff() = default;
// TODO: Test chiff audio.
    void init(const float &frequency, const float &chiffGain, const size_t &surroundingFrames) {
        const auto invertedFrequency = 1.0f / frequency;
        resonatorDelay = SAMPLE_RATE_F * invertedFrequency;
        // TODO: Why times four?
        lowPassFilter.calculateCoefficients(std::fmin(NYQUIST_WITH_MARGIN * SAMPLE_RATE_F, frequency * 4.0f));
        envelope.init({5.0f * invertedFrequency, 100.0f * invertedFrequency, 0.01f, 100.0f * invertedFrequency});
        chiffDelayFrameCount = static_cast<size_t>(std::min<float>(static_cast<float>(DelayLineStatic<SAMPLE_RATE / 2, PROCESS_FRAMES_SIZE>::size()), 0.5f * invertedFrequency * SAMPLE_RATE_F));
        framesUntilRelease = surroundingFrames + OUTPUT_CHANNELS * chiffDelayFrameCount;
        gain = chiffGain;
    }

    void process(const PipeState &state, std::array<float, PROCESS_FRAMES_SIZE> &buffer);
    void release() {
        noiseEnvelope.release();
        envelope.release();
    }
    [[nodiscard]] size_t getDelaySampleCount() const { return chiffDelayFrameCount; }
    [[nodiscard]] bool isOver() const noexcept { return framesUntilRelease == 0; }

private:
    DelayLineStatic<SAMPLE_RATE / 2, PROCESS_FRAMES_SIZE> delayLine{};
    DelayLineStatic<SAMPLE_RATE / 2, PROCESS_FRAMES_SIZE> pipeResonator{};
    LowPassFilter lowPassFilter{};
    dsp::Envelope noiseEnvelope{{0.01f, 0.0f, 1.0f, 0.02f}};
    dsp::Envelope envelope{};
    float gain{};
    float resonatorDelay{};
    size_t chiffDelayFrameCount{};
    size_t framesUntilRelease{};  ///< Counter to account for the delayed sound before recycling the voice.
    std::array<float, PROCESS_FRAMES_SIZE> swapBuffer{};
};