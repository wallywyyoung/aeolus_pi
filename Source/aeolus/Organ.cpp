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

#include <fstream>
#include <memory>

#include "IOManager.h"
#include "aeolus/EngineGlobal.h"
#include "aeolus/DivisionFactory.h"

Organ::Organ() : _voicePool(std::make_shared<VoicePool>(*this)) {
    populateDivisions();
}

void Organ::prepareToPlay() {
    // Select the first IR for reverb by default
    setReverbIR(_selectedIR);
    _convolver.setDryWet(1.0f, 0.25f, true);
}

void Organ::setReverbIR(const int num) {
    if (const auto&[irs, longestIRLength] = EngineGlobal::getInstance()->getIRs(); num >= 0 && num < irs.size()) {
        const auto& ir = irs[num];
        _convolver.setLength(static_cast<int>(ir.getNumSamples() / dsp::Convolver::BlockSize + 1) * dsp::Convolver::BlockSize);
        _convolver.setIR(ir);
        _convolver.prepareToPlay(); // these parameters are irrelevant
        _convolver.setZeroDelay(ir.zeroDelay);
        _reverbTailCounter = _convolver.length();
        _selectedIR = num;
    }
}

void Organ::setReverbWet(const float v) { _convolver.setDryWet(1.0f, v); }

void Organ::setVolume(const float v, const bool immediate) { _volume.setValue(v, immediate); }

void Organ::setDivisionNoteOn(const int &division, const int &note) {
    clearDivisionsTriggerFlag();
    _divisions[division]->setNoteOn(note, false);
}

void Organ::setDivisionNoteOff(const int &division, const int &note) {
    clearDivisionsTriggerFlag();
    _divisions[division]->setNoteOff(note, false);
}

void Organ::setDivisionAllNotesOff(const int& division) {
    _divisions[division]->setAllNotesOff(false);
}

void Organ::setGlobalAllNotesOff() {
    for (const auto &division : _divisions) {
        division->setAllNotesOff(false);
    }
}

void Organ::handleDivisionSwell(const int& division, const float& value) {
    _divisions[division]->handleSwell(value);
};

void Organ::setDivisionStopOn(const int& division, const int& stop) {
    _divisions[division]->setStopOn(stop);
}

void Organ::setDivisionStopOff(const int& division, const int& stop) {
    _divisions[division]->setStopOff(stop);
}

void Organ::setDivisionStopToggle(const int& division, const int& stop) {
    _divisions[division]->setStopToggle(stop);
}

void Organ::setDivisionAllStopsOff(const int& division) {
    _divisions[division]->setAllStopsOff();
}

void Organ::setDivisionAllStopsOn(const int& division) {
    _divisions[division]->setAllStopsOn();
}

void Organ::setGlobalAllStopsOff() {
    for (auto& division : _divisions) {
        division->setAllStopsOff();
    }
}

void Organ::setGlobalAllStopsOn() {
    for (auto& division : _divisions) {
        division->setAllStopsOn();
    }
}

void Organ::setDivisionCouplerOn(const int& division, const int& coupler) {
    _divisions[division]->setCouplerOn(coupler);
}

void Organ::setDivisionCouplerOff(const int& division, const int& coupler) {
    _divisions[division]->setCouplerOff(coupler);
}

void Organ::setDivisionTremulantOn(const int& division) {
    _divisions[division]->setTremulantOn();
}

void Organ::setDivisionTremulantOff(const int& division) {
    _divisions[division]->setTremulantOff();
}

void Organ::setDivisionPiston(const int& division, const int& piston) {
    _divisions[division]->setPiston(piston);
}

void Organ::recallDivisionPiston(const int& division, const int& piston) {
    _divisions[division]->recallPiston(piston);
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
        _divisions[i]->recallPiston(piston.divisions[i]);
    }
}

GlobalPiston Organ::captureStateAsPiston() const {
    GlobalPiston globalPiston{};
    globalPiston.divisions.resize(_divisions.size());
    for (const auto& division : _divisions) {
        globalPiston.divisions.emplace_back(division->captureStateAsPiston());
    }
    return globalPiston;
}

Division *Organ::getDivisionByName(const std::string &name) const {
    for (auto &division : _divisions) {
        if (division->getName() == name)
            return division.get();
    }

    return nullptr;
}

void Organ::clearDivisionsTriggerFlag() const {
    for (auto& division : _divisions) {
        division->clearTriggerFlag();
    }
}

bool Organ::processSubFrame() {
    generateTremulant();

    _subFrameBuffer.clear();

    bool wasAudioGenerated = false;

    for (const auto &division : _divisions) {

        _divisionFrameBuffer.clear();

        const bool hasVoices = division->process(_divisionFrameBuffer, _voiceFrameBuffer);
        wasAudioGenerated |= hasVoices;

        if (!hasVoices) {
            division->modulate(_divisionFrameBuffer, _tremulantBuffer);

            for (int ch = 0; ch < _subFrameBuffer.getNumChannels(); ++ch) {
                _subFrameBuffer.addFrom(ch, 0, _divisionFrameBuffer, ch, 0, AUDIO_SUB_FRAME_LENGTH);
            }
        }
    }

    return wasAudioGenerated;
}

void Organ::generateTremulant() {
    float* buf = _tremulantBuffer.getWritePointer(0);
    for (int i = 0; i < AUDIO_SUB_FRAME_LENGTH; ++i) {
        const float s = sinf(_tremulantPhase);
        buf[i] = s * TREMULANT_LEVEL;
        _tremulantPhase += TREMULANT_PHASE_INCREMENT;
        if (_tremulantPhase >= std::numbers::pi_v<float> * 2) {
            _tremulantPhase -= std::numbers::pi_v<float> * 2;
        }
    }
}

void Organ::applyVolume(AudioBuffer& out) {
    if (_volume.isSmoothing()) {
        for (int i = 0; i < out.getNumSamples(); ++i) {
            const float g = _volume.nextValue() * VOLUME_GAIN;

            for (int ch = 0; ch < out.getNumChannels(); ++ch)
                out.getWritePointer(ch)[i] *= g;
        }
    } else {
        const float g = _volume.target() * VOLUME_GAIN;
        out.applyGain(g);
    }
}

void Organ::applyVolume(float* inOut, const size_t framesPerChannel) {
    if (_volume.isSmoothing()) {
        for (int i = 0; i < framesPerChannel; ++i) {
            const float g = _volume.nextValue() * VOLUME_GAIN;
            inOut[i*2] *= g;
            inOut[i*2+1] *= g;
        }
    } else {
        const float g = _volume.target() * VOLUME_GAIN;
        for (int i = 0; i < framesPerChannel; ++i) {
            inOut[i*2] *= g;
            inOut[i*2+1] *= g;
        }
    }
}

auto Organ::populateDivisions() -> void {
    const std::filesystem::path configFile = "./Resources/configs/default_organ.json";

    if (!exists(configFile)) {
        return;
    }

    std::ifstream stream(configFile);
    auto config = nlohmann::json::parse(stream);

    for (auto divisionDef : config["divisions"]) {
        auto division = DivisionFactory::initFromJson(divisionDef, *this);
        _divisions.push_back(std::move(division));
    }

    // if (config.contains("sequencer")) {
    //     auto sequencer = config["sequencer"];
    //     if (sequencer.contains("backward_key")) {
    //         auto& v= sequencer["backward_key"];
    //         _sequencerStepBackwardKeySwitches.clear();
    //         if (v.is_number_integer()) {
    //             _sequencerStepBackwardKeySwitches.push_back(v);
    //         } else if (v.is_array()) {
    //             for (const auto& key : v)
    //                 _sequencerStepBackwardKeySwitches.push_back(key);
    //         }
    //     }
    //
    //     if (sequencer.contains("forward_key")) {
    //         auto& v= sequencer["forward_key"];
    //         _sequencerStepForwardKeySwitches.clear();
    //         if (v.is_number_integer()) {
    //             _sequencerStepForwardKeySwitches.push_back(v);
    //         } else if (v.is_array()) {
    //             for (const auto& key : v)
    //                 _sequencerStepForwardKeySwitches.push_back(key);
    //         }
    //     }
    // }

    // Update division links after they've been loaded.
    for (const auto& division : _divisions) {
        division->init();
    }
}
