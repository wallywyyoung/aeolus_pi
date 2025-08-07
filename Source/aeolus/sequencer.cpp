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

#include "aeolus/sequencer.h"
#include "aeolus/engine.h"





Sequencer::Sequencer(Engine& engine, const int numSteps)
    : _engine{engine}
    , _steps(numSteps)
    , _currentStep{0}
    , _dirty{true}
{
    assert(_steps.size() > 0);

    initFromEngine();
}

void Sequencer::captureCurrentStep()
{
    captureState(_steps[_currentStep]);
    _dirty = false;
}

void Sequencer::captureStateToStep(const int index)
{
    isPositiveAndBelow(index, static_cast<int>(_steps.size()));

    captureState(_steps[index]);

    // Current state now matches the sequencer step, so we switch to it
    _currentStep = index;
    _dirty = false;
}

auto Sequencer::setStep(const int index, const bool captureCurrentState) -> void {
    assert(index >= 0 && index < static_cast<int>(_steps.size()));

    if (captureCurrentState)
        captureCurrentStep();

    _currentStep = index;
    recallState(_steps[_currentStep]);
    _dirty = false;
}

void Sequencer::stepBackward()
{
    if (_currentStep > 0)
        setStep(_currentStep - 1);
}

void Sequencer::stepForward()
{
    if (_currentStep < static_cast<int>(_steps.size()) - 1)
        setStep(_currentStep + 1);
}

void Sequencer::initFromEngine()
{
    const auto numDivisions = _engine.getDivisionCount();

    for (auto&[divisions] : _steps) {
        divisions.resize(numDivisions);

        for (int divIdx = 0; divIdx < numDivisions; ++divIdx) {
            const auto division = _engine.getDivisionByIndex(divIdx);
            divisions[divIdx].stops.resize(division->getStopsCount());
            divisions[divIdx].links.resize(division->getLinksCount());
        }
    }
}

void Sequencer::captureState(OrganState& organState)
{
    const auto numDivisions = _engine.getDivisionCount();
    assert(organState.divisions.size() == numDivisions);

    for (int divIdx = 0; divIdx < numDivisions; ++divIdx) {
        auto const division = _engine.getDivisionByIndex(divIdx);
        auto&[stops, tremulant, links] = organState.divisions[divIdx];

        const auto numStops = division->getStopsCount();
        assert(stops.size() == numStops);

        // Capture stops
        for (int stopIdx = 0; stopIdx < numStops; ++stopIdx)
            stops[stopIdx] = division->getStopByIndex(stopIdx).isEnabled();

        // Capture tremulant
        tremulant = division->isTremulantEnabled();

        // Capture links
        const auto numLinks = division->getLinksCount();
        for (int linkIdx = 0; linkIdx < numLinks; ++linkIdx)
            links[linkIdx] = division->getLinkByIndex(linkIdx).enabled;
    }
}

void Sequencer::recallState(const OrganState& organState) {
    const auto numDivisions = _engine.getDivisionCount();
    assert(organState.divisions.size() == numDivisions);

    for (int divIdx = 0; divIdx < numDivisions; ++divIdx) {
        auto const division = _engine.getDivisionByIndex(divIdx);

        assert(organState.divisions[divIdx].stops.size() == division->getStopsCount());

        // Restore stops
        for (int i = 0; i < division->getStopsCount(); ++i) {
            if (organState.divisions[divIdx].stops[i]) {
                division->setStopOn(i);
            } else {
                division->setStopOff(i);
            }
        }

        // Restore tremulant
        division->setTremulantEnabled(organState.divisions[divIdx].tremulant);

        // Restore links
        for (int i = 0; i < division->getLinksCount(); ++i)
            division->enableLink(i, organState.divisions[divIdx].links[i]);
    }
}
