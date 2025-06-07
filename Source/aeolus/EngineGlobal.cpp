//
// Created by Wally Young on 6/6/25.
//

#include "EngineGlobal.h"
#include <thread_pool/thread_pool.h>

using namespace aeolus;

EngineGlobal::EngineGlobal(IRs &irs, std::vector<Addsynth> &synths)
        : _rankwaves{}
        , _scale(Scale::EqualTemp)
        , _tuningFrequency(TUNING_FREQUENCY_DEFAULT)
        , _globalProperties{}
        , _mtsClient{ nullptr }
{
    _mtsClient = MTS_RegisterClient();

    PropertiesFile::Options options{};

    options.applicationName = ProjectInfo::projectName;
    options.filenameSuffix = ".settings";
    //options.osxLibrarySubFolder = "~/Library/Application Support";
    options.osxLibrarySubFolder = "Application Support";
    options.storageFormat = PropertiesFile::storeAsXML;

    _globalProperties.setStorageParameters(options);

    loadSettings();

    loadRankwaves(synths);
    this->irs = irs;

    startTimer(100);
}

EngineGlobal::~EngineGlobal() {
    if (_mtsClient != nullptr) {
        MTS_DeregisterClient(_mtsClient);
    }

    saveSettings();
}

void EngineGlobal::loadSettings()
{
    if (auto* propertiesFile = _globalProperties.getUserSettings()) {
        const float tuningFreq = (float)propertiesFile->getDoubleValue(settings::tuningFrequency, TUNING_FREQUENCY_DEFAULT);

        if (tuningFreq >= TUNING_FREQUENCY_MIN && tuningFreq <= TUNING_FREQUENCY_MAX)
            _tuningFrequency = tuningFreq;

        const int scaleType = propertiesFile->getIntValue(settings::tuningTemperament, (int)Scale::EqualTemp);

        if (scaleType >= (int)Scale::First && scaleType < (int)Scale::Total)
            _scale.setType(static_cast<Scale::Type>(scaleType));

        setMTSEnabled(propertiesFile->getBoolValue(settings::mtsEnabled, false));
    }
}

void EngineGlobal::saveSettings()
{
    if (auto* propertiesFile = _globalProperties.getUserSettings()) {
        propertiesFile->setValue(settings::tuningFrequency, _tuningFrequency);
        propertiesFile->setValue(settings::tuningTemperament, (int)_scale.getType());
        propertiesFile->setValue(settings::mtsEnabled, _mtsEnabled);
    }

    _globalProperties.saveIfNeeded();
}

std::vector<std::string> EngineGlobal::getAllStopNames() const
{
    auto names = std::vector<std::string>();

    for (const auto rankwave : _rankwaves) {
        names.push_back(rankwave.getStopName());
    }

    return names;
}

Rankwave* EngineGlobal::getStopByName(const std::string& name)
{
    if (!_rankwavesByName.contains(name))
        return nullptr;

    return _rankwavesByName[name];
}

void EngineGlobal::updateStops(float sampleRate)
{
    _sampleRate = sampleRate;

    dp::thread_pool pool(_rankwaves.size());

    for (auto& rw : _rankwaves) {
        auto *rwp = &rw;
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
        return _scale.getFrequencyForMidoNote(midiNote);
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
    for (auto& rw : _rankwaves) {
        rw.retunePipes(_scale, _tuningFrequency);
    }

    // @note We don't kill active voices - they will be using pipes from a parallel set.
    //       However, switching tuning very fast (while keeping the voice sustained)
    //       may result in voice to be killed.

    updateStops(_sampleRate);
}

void EngineGlobal::loadRankwaves(std::vector<Addsynth> &synths) {
    auto& model = Model(synths);

    for (int i = 0; i < model.getStopsCount(); ++i) {
        auto* synth = model[i];
        assert(synth);

        auto rankwave = std::make_unique<Rankwave>(*synth);
        rankwave->createPipes(_scale, _tuningFrequency);

        auto* ptr = rankwave.get();
        _rankwaves.push_back(rankwave.release());
        _rankwavesByName.insert(ptr->getStopName(), ptr);
    }
}

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
