//
// Created by Wally Young on 6/6/25.
//

#include "EngineGlobal.h"
#include "aeolus/engine.h"
#include <thread_pool/thread_pool.h>
#include "../IOManager.h"

using namespace aeolus;

EngineGlobal::EngineGlobal() : _rankwavesByName{}, _scale(Scale::EqualTemp), _tuningFrequency(TUNING_FREQUENCY_DEFAULT), _mtsClient {nullptr}, engine(), midiManager(), model() {
    midiManager.addListener(dynamic_cast<MidiManager::MidiListener*>(&engine));
    irs = IOManager::loadIRs();
    loadRankwaves();
}

EngineGlobal::~EngineGlobal() {
    if (_mtsClient != nullptr) {
        MTS_DeregisterClient(_mtsClient);
    }
}

std::vector<std::string> EngineGlobal::getAllStopNames() const
{
    auto names = std::vector<std::string>();

    for (const auto &rankwave : _rankwavesByName) {
        names.push_back(rankwave.first);
    }

    return names;
}

std::shared_ptr<Rankwave> EngineGlobal::getStopByName(const std::string& name)
{
    return _rankwavesByName[name];
}

void EngineGlobal::updateStops(float sampleRate)
{
    _sampleRate = sampleRate;

    dp::thread_pool pool(_rankwavesByName.size());

    for (auto& rw : _rankwavesByName) {
        auto rwp = rw.second;
        pool.enqueue_detach([sampleRate, rwp]() {
            rwp->prepareToPlay(sampleRate);
        });
    }

    pool.wait_for_tasks();
/*
    // Single-thread equivalent
    for (auto* rw : _rankwaves)
        rw->prepareToPlay(sampleRate);
*/
}

void process(const std::vector<MidiMessage>& messages, AudioBuffer& buffer) {

}

void EngineGlobal::processMidi(const std::vector<MidiMessage>& messages) {
    for (const auto& message : messages) {
        midiManager.processMidiEvent(message);
    }
}

bool EngineGlobal::isConnectedToMTSMaster() {
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

float EngineGlobal::getMTSNoteToFrequency(int midiNote, int midiChannel) {
    if (_mtsClient == nullptr || !isConnectedToMTSMaster()) {
        return _scale.getFrequencyForMidiNote(midiNote);
    }

    return static_cast<float>(MTS_NoteToFrequency(_mtsClient, (char)midiNote, (char)midiChannel));
}

bool EngineGlobal::shouldMTSFilterNote(int midiNote, int midiChannel) {
    if (nullptr == _mtsClient || !isConnectedToMTSMaster()) {
        return false;
    }
    return MTS_ShouldFilterNote(_mtsClient, (char)midiNote, (char)midiChannel);
}

void EngineGlobal::setMTSEnabled(bool shouldBeEnabled) {
    _mtsEnabled = shouldBeEnabled;

    if (_mtsEnabled && nullptr == _mtsClient) {
        _mtsClient = MTS_RegisterClient();
    } else if (!_mtsEnabled && nullptr != _mtsClient) {
        MTS_DeregisterClient(_mtsClient);
        _mtsClient = nullptr;
    }
}

void EngineGlobal::rebuildRankwaves()
{
    // Prepare all the rankwaves to be retuned
    for (auto& rw : _rankwavesByName) {
        rw.second->retunePipes(_scale, _tuningFrequency);
    }

    // @note We don't kill active voices - they will be using pipes from a parallel set.
    //       However, switching tuning very fast (while keeping the voice sustained)
    //       may result in voice to be killed.

    updateStops(_sampleRate);
}

void EngineGlobal::loadRankwaves() {
    for (int i = 0; i <  model.getStopsCount(); ++i) {
        auto synth = model[i];
        auto rankwave = Rankwave(synth, _scale, _tuningFrequency);
        _rankwavesByName.emplace(rankwave.getStopName(),std::make_shared<Rankwave>(rankwave));
    }
}

const int& EngineGlobal::getMIDISwellChannelsMask() {
    return midiManager.getMIDISwellChannelsMask();
};

bool EngineGlobal::updateMTSTuningCache() {
    bool changed{};

    for (int midiNote = 0; midiNote < _mtsTuningCache.size(); ++midiNote) {
        const float f{ getMTSNoteToFrequency(midiNote) };
        if (_mtsTuningCache[midiNote] != f) {
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

    auto changed{ updateMTSTuningCache() };

    if (changed) {
        rebuildRankwaves();
    }
}
