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

#include "aeolus/dsp/Chiff.h"

#include <array>
#include <random>

void Chiff::process(const PipeState &state, std::array<float, PROCESS_FRAMES_SIZE> &buffer) {
    if (state.envelopeState == PipeState::OVER) {
        framesUntilRelease -= std::min(framesUntilRelease, static_cast<size_t>(PROCESS_FRAMES_SIZE));
    }

    delayLine.process(buffer, chiffDelayFrameCount);
    for (float &sample : buffer) {
        sample *= state.outputGain;
    }

    if (envelope.state() != dsp::Envelope::Off) {
        return; // Don't care about the noise envelope here
    }

    const float envelopeLevel{ envelope.next() * gain };
    if (envelopeLevel < 1e-4f) {
        return; // Noise is too quiet
    }

    pipeResonator.processLerpMono(swapBuffer, resonatorDelay);
    lowPassFilter.process(swapBuffer.data(), swapBuffer.data());
    if (noiseEnvelope.state() == dsp::Envelope::Sustain && envelope.state() == dsp::Envelope::Sustain) {
        SimdUtilities::multiplyRandomFactorAdditive(buffer, noiseEnvelope.level());
        pipeResonator.writeBuffer(swapBuffer);
        SimdUtilities::multiplyFactorAdditive(buffer, swapBuffer, envelopeLevel);
    } else {
        SimdUtilities::multiplyRandomEnvelopeAdditive(buffer, noiseEnvelope);
        pipeResonator.writeBuffer(swapBuffer);
        SimdUtilities::multiplyFactorEnvelopeAdditive(buffer, swapBuffer, gain, envelope);
    }
}