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

#include "aeolus/dsp/Envelope.h"

#include <cmath>

#include "MemoryConstants.h"


namespace dsp {

// Ported from
// https://www.earlevel.com/main/2013/06/03/envelope-generators-adsr-code/

Envelope::Envelope(const Trigger& trigger) {
    sustainLevel = trigger.sustain;

    attackRate = trigger.attack * SAMPLE_RATE_F;
    attackCoef = calculate(attackRate, AttackTargetRatio);
    attackBase = (1.0f + AttackTargetRatio) * (1.0f - attackCoef);

    decayRate = trigger.decay * SAMPLE_RATE_F;
    decayCoef = calculate(decayRate, DecayReleaseTargetRatio);
    decayBase = (sustainLevel - DecayReleaseTargetRatio) * (1.0f - decayCoef);

    releaseRate = trigger.release * SAMPLE_RATE_F;
    releaseCoef = calculate(releaseRate, DecayReleaseTargetRatio);
    releaseBase = -DecayReleaseTargetRatio * (1.0f - releaseCoef);
}

void Envelope::release() {
    if (currentState != Off) {
        currentState = Release;
    }
}

void Envelope::release(const float t) {
    releaseRate = t * SAMPLE_RATE_F;
    releaseCoef = calculate(releaseRate, DecayReleaseTargetRatio);
    releaseBase = -DecayReleaseTargetRatio * (1.0f - releaseCoef);

    currentState = Release;
}

float Envelope::next() {
    switch (currentState)
    {
    case Off:
        break;
    case Attack:
        currentLevel = attackBase + currentLevel * attackCoef;

        if (currentLevel >= 1.0f) {
            currentLevel = 1.0f;
            currentState = Decay;
        }
        break;
    case Decay:
        currentLevel = decayBase + currentLevel * decayCoef;

        if (currentLevel <= sustainLevel) {
            currentLevel = sustainLevel;
            currentState = currentLevel > 0.0f ? Sustain : Off;
        }
        break;
    case Sustain:
        break;
    case Release:
        currentLevel = releaseBase + currentLevel * releaseCoef;

        if (currentLevel <= 0.0f) {
            currentLevel = 0.0f;
            currentState = Off;
        }
        break;
    default:
        break;
    }

    return currentLevel;
}

float Envelope::calculate(const float rate, const float targetRatio) {
    return rate <= 0 ? 0.0f : std::exp(-std::log((1.0f + targetRatio) / targetRatio) / rate);
}

} // namespace dsp


