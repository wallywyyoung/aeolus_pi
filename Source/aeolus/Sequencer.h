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
#include "aeolus/Organ.h"

/**
 * A sequence of organ divisions states (including the stops and tremulant state).
 */
class Sequencer {
public:
    constexpr static int SEQUENCER_BACKWARD_MIDI_KEY = 22;
    constexpr static int SEQUENCER_FORWARD_MIDI_KEY = 23;

    Sequencer() = delete;
    explicit Sequencer(Organ& engine, int numSteps = 32);

    [[nodiscard]] int getStepsCount() const noexcept { return static_cast<int>(steps.size()); }
    [[nodiscard]] int getCurrentStep() const noexcept { return currentStep; }
    void captureStateToCurrentStep();
    void captureStateToStep(int index);
    void setStep(int index, bool captureCurrentState = true);
    void stepBackward();
    void stepForward();
    void setCurrentStepDirty() noexcept { dirty = true; } ///< Set this any time a division changes a value that affects DivisionCoupler.
    [[nodiscard]] bool isCurrentStepDirty() const noexcept { return dirty; }

private:
    Organ& engine;
    std::vector<Organ::GlobalPiston> steps;
    std::atomic<int> currentStep;
    bool dirty;
};


