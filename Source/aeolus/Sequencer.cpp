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

#include "aeolus/Sequencer.h"
#include <cassert>

Sequencer::Sequencer(Organ& engine, const int numSteps): engine{engine}, steps(numSteps), currentStep{0}, dirty{true} { }

void Sequencer::captureStateToCurrentStep() {
    steps[currentStep] = engine.captureStateAsPiston();
    dirty = false;
}

void Sequencer::captureStateToStep(const int index) {
    assert(index < steps.size() && index >= 0);

    steps[index] = engine.captureStateAsPiston();

    // Current state now matches the sequencer step, so we switch to it
    currentStep = index;
    dirty = false;
}

auto Sequencer::setStep(const int index, const bool captureCurrentState) -> void {
    assert(index >= 0 && index < static_cast<int>(steps.size()));
    if (captureCurrentState) {
        captureStateToCurrentStep();
    }

    currentStep = index;
    engine.recallGlobalPiston(steps[currentStep]);
    dirty = false;
}

void Sequencer::stepBackward() {
    if (currentStep > 0) {
        setStep(currentStep - 1);
    }
}

void Sequencer::stepForward() {
    if (currentStep < static_cast<int>(steps.size()) - 1) {
        setStep(currentStep + 1);
    }
}
