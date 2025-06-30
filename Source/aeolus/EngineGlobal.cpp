//
// Created by Wally Young on 6/6/25.
//

#include "IOManager.h"
#include "aeolus/EngineGlobal.h"
#include "aeolus/engine.h"
#include <thread_pool/thread_pool.h>



EngineGlobal::EngineGlobal() : engine(*this), _scale(std::make_shared<Scale>(Scale(Scale::EqualTemp))), _tuningFrequency(TUNING_FREQUENCY_DEFAULT),
                               _sampleRate(SAMPLE_RATE_F) {
    midiManager.addListener(&engine);
    irs = IOManager::loadIRs();
    loadRankwaves();
    updateStops();
    engine.prepareToPlay(_sampleRate);
}

EngineGlobal::~EngineGlobal() {
    if (_mtsClient != nullptr) {
        MTS_DeregisterClient(_mtsClient);
    }
}

const float EngineGlobal::getMTSNoteToFrequency(int midiNote, int midiChannel) const {
    if (_mtsClient == nullptr || !isConnectedToMTSMaster()) {
        return _scale->getFrequencyForMidiNote(midiNote);
    }
    return static_cast<float>(MTS_NoteToFrequency(_mtsClient, static_cast<char>(midiNote), static_cast<char>(midiChannel)));
}

std::vector<std::string> EngineGlobal::getAllStopNames() const
{
    auto names = std::vector<std::string>();

    for (const auto &rankwave : _rankwavesByName) {
        names.push_back(rankwave.first);
    }

    return names;
}

void EngineGlobal::updateStops() const {
    dp::thread_pool pool(_rankwavesByName.size());
    for (auto& rw : _rankwavesByName) {
        auto rwp = rw.second;
        pool.enqueue_detach([rwp]() {
            rwp->prepareToPlay(SAMPLE_RATE_F);
        });
    }
    pool.wait_for_tasks();
}

void process(const std::vector<MidiMessage>& messages, AudioBuffer& buffer) {

}

void EngineGlobal::processMidi(const std::vector<MidiMessage>& messages) {
    for (const auto& message : messages) {
        midiManager.processMidiEvent(message);
    }
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

void EngineGlobal::rebuildRankwaves()
{
    // Prepare all the rankwaves to be retuned
    for (const auto &val: _rankwavesByName | std::views::values) {
        val->retunePipes(*_scale, _tuningFrequency);
    }

    // @note We don't kill active voices - they will be using pipes from a parallel set.
    //       However, switching tuning very fast (while keeping the voice sustained)
    //       may result in voice to be killed.
    updateStops();
}

void EngineGlobal::loadRankwaves() {
    for (int i = 0; i <  model.getStopsCount(); ++i) {
        auto rankwave = Rankwave(model[i], *_scale, _tuningFrequency, *this);
        _rankwavesByName.emplace(rankwave.getStopName(),std::make_shared<Rankwave>(rankwave));
    }
}

const int EngineGlobal::getMIDISwellChannelsMask() const{
    return midiManager.getMIDISwellChannelsMask();
}

const bool EngineGlobal::shouldMTSFilterNoteByChannel(int midiNote, int midiChannel) const {
    if (nullptr == _mtsClient || !isConnectedToMTSMaster()) {
        return false;
    }
    return MTS_ShouldFilterNote(_mtsClient, static_cast<char>(midiNote), static_cast<char>(midiChannel));
}

bool EngineGlobal::updateMTSTuningCache() {
    bool changed{};

    for (int midiNote = 0; midiNote < _mtsTuningCache.size(); ++midiNote) {
        const float f{ getMTSNoteToFrequency(midiNote, -1) };
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
    if (updateMTSTuningCache()) {
        rebuildRankwaves();
    }
}
