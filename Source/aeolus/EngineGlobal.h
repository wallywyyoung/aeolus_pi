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

#include "mts/libMTSClient.h"
#include "globals.h"
#include "rankwave.h"
#include "scale.h"
#include <unordered_map>

AEOLUS_NAMESPACE_BEGIN

/**
 * @brief A global shared instance of the organ engine.
 *
 * This class in a singleton which is shared among all the plugin instances.
 */

class EngineGlobal {
public:
    /**
     * Impulse response descriptor for IRs embedded as binary resources.
     */
    EngineGlobal(IRs &irs, std::vector<Addsynth> &synths);

    void loadSettings();
    void saveSettings();

    int getStopsCount() const noexcept { return _rankwaves.size(); }
    Rankwave* getStop(int i) { return &_rankwaves[i]; }

    std::vector<std::string> getAllStopNames() const;
    Rankwave* getStopByName(const std::string& name);

    [[nodiscard]] const IRs& getIRs() const noexcept { return irs; }
    int getLongestIRLength() const noexcept { return _longestIRLength; }

    void updateStops(float sampleRate);

    [[nodiscard]] float getTuningFrequency() const noexcept { return _tuningFrequency; }
    void setTuningFrequency(float f) noexcept { _tuningFrequency = f; }

    [[nodiscard]] const Scale& getScale() const noexcept { return _scale; }
    void setScaleType(Scale::Type type) noexcept { _scale.setType(type); }

    bool isConnectedToMTSMaster();
    std::string getMTSScaleName();
    float getMTSNoteToFrequency(int midiNote, int midiChannel = -1);
    bool shouldMTSFilterNote(int midiNote, int midiChannel = -1);

    [[nodiscard]] bool isMTSEnabled() const { return _mtsEnabled; }
    void setMTSEnabled(bool shouldBeEnabled);

    void rebuildRankwaves();

private:
    ~EngineGlobal() ;

    void loadRankwaves(std::vector<Addsynth> &synths);

    /**
     * Refresh MTS tuning table for all MIDI notes.
     * Returns true if there was a change to the tuning.
     */
    bool updateMTSTuningCache();

    // juce::Timer
    void timerCallback();

    std::vector<Rankwave> _rankwaves;
    std::unordered_map<std::string, Rankwave*> _rankwavesByName;

//    std::vector<IR> _irs;
    IRs irs;
    int _longestIRLength;   ///< Longest IR length in samples

    float _sampleRate;
    Scale _scale;
    float _tuningFrequency;

    MTSClient* _mtsClient{};
    bool _mtsEnabled{};
    std::array<float, 128> _mtsTuningCache{};

    float _uiScalingFactor{ UI_SCALING_DEFAULT };

    juce::ApplicationProperties _globalProperties;
};

AEOLUS_NAMESPACE_END
