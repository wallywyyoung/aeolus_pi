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

#include "aeolus/EngineGlobal.h"
#include "IOManager.h"
#include "aeolus/engine.h"

#include <thread_pool/thread_pool.h>

EngineGlobal::EngineGlobal() : _scale(std::make_shared<Scale>(Scale(Scale::EqualTemp))), _tuningFrequency(TUNING_FREQUENCY_DEFAULT) { }

void EngineGlobal::init() {
    irs = IOManager::loadIRs();
    loadRankwaves();
    updateStops();
    engine = new Engine();
    engine->prepareToPlay();
    engine->setGlobalAllStopsOn();
    engine->setVolume(0.005f, true);
}

EngineGlobal::~EngineGlobal() {
    if (_mtsClient != nullptr) {
        MTS_DeregisterClient(_mtsClient);
    }
}

const float EngineGlobal::getMTSNoteToFrequency(const int midiNote, const int midiChannel) const {
    if (_mtsClient == nullptr || !isConnectedToMTSMaster()) {
        return _scale->getFrequencyForMidiNote(midiNote);
    }
    return static_cast<float>(MTS_NoteToFrequency(_mtsClient, static_cast<char>(midiNote), static_cast<char>(midiChannel)));
}

std::vector<std::string> EngineGlobal::getAllStopNames() const
{
    auto names = std::vector<std::string>();

    for (const auto &key: _rankwavesByName | std::views::keys) {
        names.push_back(key);
    }

    return names;
}

void EngineGlobal::updateStops() const {
    dp::thread_pool pool(_rankwavesByName.size());
    for (const auto &val: _rankwavesByName | std::views::values) {
        auto rwp = val.get();
        pool.enqueue_detach([rwp] {
            MemoryUtilities::enableFlushToZero();
            rwp->prepareToPlay();
            MemoryUtilities::disableFlushToZero();
        });
    }
    pool.wait_for_tasks();
}

bool EngineGlobal::isConnectedToMTSMaster() const {
    if (nullptr == _mtsClient) {
        return false;
    }
    return MTS_HasMaster(_mtsClient);
}

std::string EngineGlobal::getMTSScaleName() {
    if (_mtsClient == nullptr) {
        return {};
    }

    return std::string(MTS_GetScaleName(_mtsClient));
}

void EngineGlobal::setMTSEnabled(const bool shouldBeEnabled) {
    _mtsEnabled = shouldBeEnabled;

    if (_mtsEnabled && nullptr == _mtsClient) {
        _mtsClient = MTS_RegisterClient();
    } else if (!_mtsEnabled && nullptr != _mtsClient) {
        MTS_DeregisterClient(_mtsClient);
        _mtsClient = nullptr;
    }
}

void EngineGlobal::rebuildRankwaves() {
    // Prepare all the rankwaves to be retuned
    for (const auto &val: _rankwavesByName | std::views::values) {
        val->retunePipes(*_scale, _tuningFrequency);
    }

    // @note We don't kill active voices - they will be using pipes from a parallel set.
    //       However, switching tuning very fast (while keeping the voice sustained)
    //       may result in voice to be killed.
    updateStops();
}

void EngineGlobal::pushMidi(const MidiData &midi) {
    engine->push(midi);
}

void EngineGlobal::pushMidi(const std::vector<MidiData> &midi) {
    engine->push(midi);
}

void EngineGlobal::loadRankwaves() {
    for (int i = 0; i <  model.getStopsCount(); ++i) {
        // TODO: Fix this mapping in JSON.
        _rankwavesByName.emplace(model[i].getFileName(), std::make_unique<Rankwave>(model[i], *_scale, _tuningFrequency));
    }
}

const bool EngineGlobal::shouldMTSFilterNoteByChannel(const int midiNote, const int midiChannel) const {
    if (nullptr == _mtsClient || !isConnectedToMTSMaster()) {
        return false;
    }
    return MTS_ShouldFilterNote(_mtsClient, static_cast<char>(midiNote), static_cast<char>(midiChannel));
}

bool EngineGlobal::updateMTSTuningCache() {
    bool changed{};

    for (int midiNote = 0; midiNote < _mtsTuningCache.size(); ++midiNote) {
        if (const float f{ getMTSNoteToFrequency(midiNote, -1) }; _mtsTuningCache[midiNote] != f) {
            _mtsTuningCache[midiNote] = f;
            changed = true;
        }
    }

    return changed;
}

void EngineGlobal::timerCallback() {
    if (!_mtsEnabled) {
        return;
    }
    if (updateMTSTuningCache()) {
        rebuildRankwaves();
    }
}
