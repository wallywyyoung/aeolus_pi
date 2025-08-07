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

#include "aeolus/engine.h"

#include <fstream>

#include "IOManager.h"
#include "aeolus/EngineGlobal.h"
#include "DivisionFactory.h"

Engine::Engine() : _divisionGain(std::make_shared<AudioParameter>(1)), _voicePool(std::make_shared<VoicePool>(*this)), _params{NUM_PARAMS}, _remainedSamples{0}, _tremulantPhase{0.0f}, _selectedIR{0}, _reverbTailCounter{0}
{
    populateDivisions();
    // Sequencer can be created only after the divisions have been populated.
    _sequencer = std::make_unique<Sequencer>(*this, SEQUENCER_N_STEPS);
}

void Engine::prepareToPlay()
{
    // Select the first IR for reverb by default
    setReverbIR(_selectedIR);
    _convolver.setDryWet(1.0f, 0.25f, true);
}

void Engine::setReverbIR(const int num)
{
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

void Engine::setReverbWet(const float v) { _convolver.setDryWet(1.0f, v); }

void Engine::setVolume(const float v, const bool immediate) { _params[VOLUME].setValue(v, immediate); }

void Engine::handleSequencerSwitch(const int& note) {
    clearDivisionsTriggerFlag();
    if (isKeySwitchBackward(note)) {
        _sequencer->stepBackward();
    } else if (isKeySwitchForward(note)) {
        _sequencer->stepForward();
    }
}

void Engine::setDivisionNoteOn(const int &division, const int &note) {
    clearDivisionsTriggerFlag();
    // Ignore note-on event if filtered by MTS.
    if (EngineGlobal::getInstance()->shouldMTSFilterNoteByChannel(note, division)) {
        return;
    }
    _divisions[division]->setNoteOn(note, false);
}

void Engine::setDivisionNoteOff(const int &division, const int &note) {
    clearDivisionsTriggerFlag();
    _divisions[division]->setNoteOff(note, false);
}

void Engine::setDivisionAllNotesOff(const int& division) {
    _divisions[division]->setAllNotesOff(false);
}

void Engine::setGlobalAllNotesOff() {
    for (const auto &division : _divisions) {
        division->setAllNotesOff(false);
    }
}

void Engine::handleDivisionSwell(const int& division, const float& value) {
    _divisions[division]->handleSwell(value);
};

void Engine::handleDivisionTremulant(const int& division, const float& value) {
    _divisions[division]->handleTremulant(value);
};

void Engine::setDivisionStopOn(const int& division, const int& stop) {
    _divisions[division]->setStopOn(stop);
}

void Engine::setDivisionStopOff(const int& division, const int& stop) {
    _divisions[division]->setStopOff(stop);
}

void Engine::setDivisionStopToggle(const int& division, const int& stop) {
    _divisions[division]->setStopToggle(stop);
}

void Engine::setDivisionAllStopsOff(const int& division) {
    _divisions[division]->setAllStopsOff();
}

void Engine::setDivisionAllStopsOn(const int& division) {
    _divisions[division]->setAllStopsOn();
}

void Engine::setGlobalAllStopsOff() {
    for (auto& division : _divisions) {
        division->setAllStopsOff();
    }
}

void Engine::setGlobalAllStopsOn() {
    for (auto& division : _divisions) {
        division->setAllStopsOn();
    }
}

std::set<int> Engine::getKeySwitches() const
{
    std::set<int> keySwitches{};

    for (int key : _sequencerStepBackwardKeySwitches)
        keySwitches.insert(key);

    for (int key : _sequencerStepForwardKeySwitches)
        keySwitches.insert(key);

    return keySwitches;
}

Division *Engine::getDivisionByName(const std::string &name) const {
    for (auto &division : _divisions) {
        if (division->getName() == name)
            return division.get();
    }

    return nullptr;
}

void Engine::clearDivisionsTriggerFlag() const {
    for (auto& division : _divisions) {
        division->clearTriggerFlag();
    }
}

bool Engine::processSubFrame() {
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

    _remainedSamples = AUDIO_SUB_FRAME_LENGTH;

    return wasAudioGenerated;
}

void Engine::generateTremulant()
{
    float* buf = _tremulantBuffer.getWritePointer(0);

    for (int i = 0; i < AUDIO_SUB_FRAME_LENGTH; ++i) {
        const float s = sinf(_tremulantPhase);
        buf[i] = s * TREMULANT_LEVEL;
        _tremulantPhase += TREMULANT_PHASE_INCREMENT;

        if (_tremulantPhase >= std::numbers::pi_v<float> * 2)
            _tremulantPhase -= std::numbers::pi_v<float> * 2;
    }
}

void Engine::applyVolume(AudioBuffer& out)
{
    if (_params[VOLUME].isSmoothing()) {
        for (int i = 0; i < out.getNumSamples(); ++i) {
            const float g = _params[VOLUME].nextValue() * VOLUME_GAIN;

            for (int ch = 0; ch < out.getNumChannels(); ++ch)
                out.getWritePointer(ch)[i] *= g;
        }
    } else {
        const float g = _params[VOLUME].target() * VOLUME_GAIN;
        out.applyGain(g);
    }
}

void Engine::applyVolume(float* inOut, const size_t framesPerChannel)
{
    if (_params[VOLUME].isSmoothing()) {
        for (int i = 0; i < framesPerChannel; ++i) {
            const float g = _params[VOLUME].nextValue() * VOLUME_GAIN;
            inOut[i*2+1] *= g;
            inOut[i*2+1] *= g;
        }
    } else {
        const float g = _params[VOLUME].target() * VOLUME_GAIN;
        for (int i = 0; i < framesPerChannel; ++i) {
            inOut[i*2+1] *= g;
            inOut[i*2+1] *= g;
        }
    }
}

bool Engine::isKeySwitchForward(const int key) const
{
    return std::ranges::find(_sequencerStepForwardKeySwitches, key) != _sequencerStepForwardKeySwitches.end();
}

bool Engine::isKeySwitchBackward(const int key) const
{
    return std::ranges::find(_sequencerStepBackwardKeySwitches, key) != _sequencerStepBackwardKeySwitches.end();
}

auto Engine::populateDivisions() -> void {
    const std::filesystem::path configFile = "./Resources/configs/default_organ.json";

    if (!exists(configFile))
        return;

    std::ifstream stream(configFile);
    auto config = nlohmann::json::parse(stream);

    for (auto divisionDef : config["divisions"]) {
        auto division = DivisionFactory::initFromJson(divisionDef, *this);
        _divisions.push_back(std::move(division));
    }

    if (config.contains("sequencer")) {
        auto sequencer = config["sequencer"];
        if (sequencer.contains("backward_key")) {
            populateKeySwitchesVector(_sequencerStepBackwardKeySwitches, sequencer["backward_key"]);
        }

        if (sequencer.contains("forward_key")) {
            populateKeySwitchesVector(_sequencerStepForwardKeySwitches, sequencer["forward_key"]);
        }
    }

    // Remove all the links if any.
    for (const auto& division : _divisions) {
        division->clearLinkedDivisions();
    }

    // Update division links after they've been loaded.
    for (const auto& division : _divisions) {
        division->populateLinkedDivisions();
    }

    // @todo Do we want the divisions to be reordered by the couplings?
}

void Engine::populateKeySwitchesVector(std::vector<int>& switches, const nlohmann::json& v) {
    if (v.is_null())
        return;

    switches.clear();

    if (v.is_number_integer()) {
        switches.push_back(v);
    } else if (v.is_array()) {
        for (const auto& key : v)
            switches.push_back(key);
    }
}
