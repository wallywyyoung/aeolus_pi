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
#include "aeolus/Organ.h"

#include <cassert>


Sequencer::Sequencer(Organ& engine, int numSteps): _engine{engine}, _steps(numSteps), _currentStep{0}, _dirty{true} { }

void Sequencer::captureCurrentStep() {
    _steps[_currentStep] = _engine.captureStateAsPiston();
    _dirty = false;
}

void Sequencer::captureStateToStep(const int index) {
    isPositiveAndBelow(index, static_cast<int>(_steps.size()));

    _steps[index] = _engine.captureStateAsPiston();

    // Current state now matches the sequencer step, so we switch to it
    _currentStep = index;
    _dirty = false;
}

auto Sequencer::setStep(const int index, const bool captureCurrentState) -> void {
    assert(index >= 0 && index < static_cast<int>(_steps.size()));
    if (captureCurrentState) {
        captureCurrentStep();
    }

    _currentStep = index;
    _engine.recallGlobalPiston(_steps[_currentStep]);
    _dirty = false;
}

void Sequencer::stepBackward() {
    if (_currentStep > 0) {
        setStep(_currentStep - 1);
    }
}

void Sequencer::stepForward() {
    if (_currentStep < static_cast<int>(_steps.size()) - 1) {
        setStep(_currentStep + 1);
    }
}
