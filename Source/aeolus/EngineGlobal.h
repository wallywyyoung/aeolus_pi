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

#pragma once

#include <iostream>
#include <unordered_map>

#include "aeolus/rankwave.h"
#include "aeolus/scale.h"
#include "aeolus/engine.h"
#include "aeolus/Model.h"
#include "mts/libMTSClient.h"
#include "MidiData.h"

/**
 * @brief A global shared instance of the organ engine.
 *
 * This class in a singleton which is shared among all the plugin instances.
 */

class EngineGlobal final {
public:
    EngineGlobal();
    void init();
    EngineGlobal(EngineGlobal const&) = delete;
    void operator=(EngineGlobal const&) = delete;
    static EngineGlobal* getInstance() noexcept {
        static EngineGlobal instance;
        return &instance;
    }

    [[nodiscard]] const int getMIDISwellChannelsMask() const;
    [[nodiscard]] const bool shouldMTSFilterNoteByChannel(int midiNote, int midiChannel) const;
    [[nodiscard]] bool isMTSEnabled() const { return _mtsEnabled; }
    [[nodiscard]] const IRs& getIRs() const noexcept { return irs; }
    [[nodiscard]] Rankwave *getStopByName(const std::string &name) const { return _rankwavesByName.at(name).get(); }
    [[nodiscard]] const float getMTSNoteToFrequency(int midiNote, int midiChannel) const;
    [[nodiscard]] int getStopsCount() const noexcept { return _rankwavesByName.size(); }
    [[nodiscard]] std::vector<std::string> getAllStopNames() const;
    [[nodiscard]] int getLongestIRLength() const noexcept { return _longestIRLength; }
    void updateStops() const;
    [[nodiscard]] float getTuningFrequency() const noexcept { return _tuningFrequency; }
    void setTuningFrequency(const float f) noexcept { _tuningFrequency = f; }
    [[nodiscard]] const Scale& getScale() const noexcept { return *_scale; }
    void setScaleType(const Scale::Type type) const noexcept { _scale->setType(type); }
    [[nodiscard]] bool isConnectedToMTSMaster() const;
    [[nodiscard]] std::string getMTSScaleName();
    void setMTSEnabled(bool shouldBeEnabled);
    void rebuildRankwaves();

    template<auto OUT_BUFFER_SIZE>
    void audioCallbackStereo(float (&out)[OUT_BUFFER_SIZE]) {
        engine->processNoninterpolatedRealtimeStereo<OUT_BUFFER_SIZE>(out);
    }

    void pushMidi(const MidiData& midi);
    void pushMidi(const std::vector<MidiData>& midi);

private:
    constexpr static float TUNING_FREQUENCY_DEFAULT = 440.0f; /// mid-A tuning frequency.
    ~EngineGlobal();
    void loadRankwaves();
    /**
     * Refresh MTS tuning table for all MIDI notes.
     * Returns true if there was a change to the tuning.
     */
    bool updateMTSTuningCache();

    // juce::Timer
    void timerCallback();
    Model model;
    Engine *engine;
    std::unordered_map<std::string, std::unique_ptr<Rankwave>> _rankwavesByName{};
    std::vector<IR> _irs{};
    IRs irs;
    MTSClient* _mtsClient{};
    std::shared_ptr<Scale> _scale;
    int _longestIRLength{};   ///< Longest IR length in samples

    float _sampleRate;
    float _tuningFrequency;

    bool _mtsEnabled{};
    std::array<float, 128> _mtsTuningCache{};
};
