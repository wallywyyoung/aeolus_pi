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

#include <atomic>
#include <vector>

#include "aeolus/globals.h"
class Organ;

/**
 * A sequence of organ divisions states (including the stops and tremulant state).
 */
class Sequencer {
public:
    constexpr static int SEQUENCER_BACKWARD_MIDI_KEY = 22;
    constexpr static int SEQUENCER_FORWARD_MIDI_KEY = 23;

    Sequencer() = delete;
    explicit Sequencer(Organ& engine, int numSteps = 32);

    [[nodiscard]] int getStepsCount() const noexcept { return static_cast<int>(_steps.size()); }
    [[nodiscard]] int getCurrentStep() const noexcept { return _currentStep; }

    /**
     * Capture the organ state into the current step.
     */
    void captureCurrentStep();

    void captureStateToStep(int index);

    void setStep(int index, bool captureCurrentState = true);

    void stepBackward();
    void stepForward();

    // Set this any time a division changes a value that affects DivisionCoupler.
    void setCurrentStepDirty() noexcept { _dirty = true; }
    [[nodiscard]] bool isCurrentStepDirty() const noexcept { return _dirty; }

private:

    Organ& _engine;
    std::vector<GlobalPiston> _steps;
    std::atomic<int> _currentStep;
    bool _dirty;
};


