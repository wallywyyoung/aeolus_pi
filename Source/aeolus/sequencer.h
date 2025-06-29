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

#include "aeolus/globals.h"

#include <atomic>
#include <vector>
#include <map>
#include <any>



class Engine;

/**
 * A sequence of organ divisions states (including the stops and tremulant state).
 */
class Sequencer
{
public:

    struct DivisionState
    {
        std::vector<bool> stops;    ///< Stops enablement mask.
        bool tremulant;             ///< Tremulant enablement.

        std::vector<bool> links;    ///< Manuals links.

//        std::map<std::string, std::any> getPersistentState() const;
//        void setPersistentState(const std::map<std::string, std::any>& v);
    };

    struct OrganState
    {
        std::vector<DivisionState> divisions;

//        std::map<std::string, std::any> getPersistentState() const;
//        void setPersistentState(const std::map<std::string, std::any>& v);
    };

    Sequencer() = delete;
    Sequencer(Engine& engine, int numSteps);

    [[nodiscard]] int getStepsCount() const noexcept { return (int)_steps.size(); }
    [[nodiscard]] int getCurrentStep() const noexcept { return _currentStep; }

//    std::map<std::string, std::any> getPersistentState() const;
//    void setPersistentState(const std::map<std::string, std::any>& v);

    /**
     * Capture the organ state from the engine into the current step.
     */
    void captureCurrentStep();

    void captureStateToStep(int index);

    void setStep(int index, bool captureCurrentState = true);

    void stepBackward();
    void stepForward();

    void setCurrentStepDirty() noexcept { _dirty = true; }
    [[nodiscard]] bool isCurrentStepDirty() const noexcept { return _dirty; }

private:

    void initFromEngine();
    void captureState(OrganState& organState);
    void recallState(const OrganState& organState);

    Engine& _engine;
    std::vector<OrganState> _steps;
    std::atomic<int> _currentStep;

    bool _dirty;
};


