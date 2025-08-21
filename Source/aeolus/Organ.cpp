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

#include "aeolus/Organ.h"
#include "aeolus/DivisionFactory.h"

#include <memory>

Organ::Organ(const std::function<std::shared_ptr<RankWave>(const std::string&)> &getStopByName) : voicePool(std::make_shared<VoicePool>()) {
    DivisionFactory::initFromJson(voicePool, divisions, getStopByName);
}

void Organ::setDivisionNoteOn(const int &division, const int &note) {
    divisions[division]->setNoteOn(note);
}

void Organ::setDivisionNoteOff(const int &division, const int &note) {
    divisions[division]->setNoteOff(note);
}

void Organ::setDivisionAllNotesOff(const int& division) {
    divisions[division]->setAllNotesOff();
}

void Organ::setGlobalAllNotesOff() {
    for (const auto &division : divisions) {
        division->setAllNotesOff();
    }
}

void Organ::handleDivisionSwell(const int& division, const float& value) {
    divisions[division]->handleSwell(value);
};

void Organ::setDivisionStopOn(const int& division, const int& stop) {
    divisions[division]->setStopOn(stop);
}

void Organ::setDivisionStopOff(const int& division, const int& stop) {
    divisions[division]->setStopOff(stop);
}

void Organ::setDivisionStopToggle(const int& division, const int& stop) {
    divisions[division]->setStopToggle(stop);
}

void Organ::setDivisionAllStopsOff(const int& division) {
    divisions[division]->setAllStopsOff();
}

void Organ::setDivisionAllStopsOn(const int& division) {
    divisions[division]->setAllStopsOn();
}

void Organ::setGlobalAllStopsOff() {
    for (const auto & division : divisions) {
        division->setAllStopsOff();
    }
}

void Organ::setGlobalAllStopsOn() {
    for (const auto & division : divisions) {
        division->setAllStopsOn();
    }
}

void Organ::setDivisionCouplerOn(const int& division, const int& coupler) {
    divisions[division]->setCouplerOn(coupler);
}

void Organ::setDivisionCouplerOff(const int& division, const int& coupler) {
    divisions[division]->setCouplerOff(coupler);
}

void Organ::setDivisionTremulantOn(const int& division) {
    divisions[division]->setTremulantOn();
}

void Organ::setDivisionTremulantOff(const int& division) {
    divisions[division]->setTremulantOff();
}

void Organ::setDivisionPiston(const int& division, const int& piston) {
    divisions[division]->setPiston(piston);
}

void Organ::recallDivisionPiston(const int& division, const int& piston) {
    divisions[division]->recallPiston(piston);
}

void Organ::setGlobalPiston(const int& piston) {
    if (pistons.size() <= piston) {
        pistons.resize(piston + 1);
    }
    pistons[piston] = captureStateAsPiston();
}

void Organ::recallGlobalPiston(const int& piston) {
    if (pistons.size() <= piston) {
        return;
    }
    recallGlobalPiston(pistons[piston]);
}

void Organ::recallGlobalPiston(const GlobalPiston& piston) {
    for (int i = 0; i < piston.divisions.size(); ++i) {
        divisions[i]->recallPiston(piston.divisions[i]);
    }
}

GlobalPiston Organ::captureStateAsPiston() const {
    GlobalPiston globalPiston{};
    globalPiston.divisions.resize(divisions.size());
    for (const auto& division : divisions) {
        globalPiston.divisions.emplace_back(division->captureStateAsPiston());
    }
    return globalPiston;
}

// TODO: Index, batch, vectorize
void Organ::generateTremulant() {
    float* buf = tremulantFrameBuffer.getWritePointer(0);
    for (int i = 0; i < AUDIO_SUB_FRAME_LENGTH; ++i) {
        const float s = sinf(tremulantPhase);
        buf[i] = s * TREMULANT_LEVEL;
        tremulantPhase += TREMULANT_PHASE_INCREMENT;
        if (tremulantPhase >= std::numbers::pi_v<float> * 2) {
            tremulantPhase -= std::numbers::pi_v<float> * 2;
        }
    }
}
