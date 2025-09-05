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
#include <cmath>
#include <random>



namespace dsp {

void Chiff::process(std::array<float, PROCESS_FRAMES_SIZE> &out) {
    thread_local std::random_device rnd;
    thread_local std::mt19937 gen(rnd());
    thread_local std::uniform_real_distribution dist(-1.0f, 1.0f);

    if (!isActive()) {
        return;
    }

    if (noiseEnvelope.state() == Envelope::Sustain && envelope.state() == Envelope::Sustain) {
        const float noiseLevel{ noiseEnvelope.level() };
        const float envelopeLevel{ envelope.level() * gain };

        if (envelopeLevel < 1e-4f) {
            return; // Noise is too quiet
        }

        for (float &outSample : out) {
            const float x0{ dist(gen) };
            const float x{ x0 * noiseLevel };
            float y{ pipeResonator.read(pipeDelay) };
            y = BiquadFilter::tick(lpSpec, lpState, y);
            y += x;
            pipeResonator.write(y);

            outSample += y * envelopeLevel;
        }

        return;
    }

    for (float &outSample : out) {
        const float x0 = dist(gen);
        const float x = x0 * noiseEnvelope.next();
        float y = pipeResonator.read(pipeDelay);
        y = BiquadFilter::tick(lpSpec, lpState, y);
        y += x;
        pipeResonator.write(y);

        outSample += y * gain * envelope.next();
    }
}

} // namespace dsp


