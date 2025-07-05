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

#include "aeolus/globals.h"
#include "aeolus/rankwave.h"
#include "aeolus/stop.h"
#include "aeolus/voice.h"
#include "aeolus/audioparam.h"
#include "aeolus/dsp/filter.h"
#include "aeolus/MidiMessage.h"
#include "AudioBuffer.h"

#include <atomic>
#include <vector>
#include <bitset>

class Engine;

/**
 * @brief Single keyboard division.
 *
 * A division may have multiple stops available, which can be enabled or
 * disabled individually.
 */
class Division
{
public:
    enum Params { GAIN = 0, NUM_PARAMS };
    /// Link with another division.
    struct Link {
        Division* division;
        bool enabled = false;
    };

    explicit Division(const Engine& engine, const std::string& name = std::string());

    /**
     * @brief Load the division configuration from a JSON object.
     *
     * This will configure the division from an organ configuration data.
     */
    // void initFromJson(const nlohmann::json& v);

    const Engine& getEngine() const noexcept { return _engine; }

    [[nodiscard]] std::string getName() const { return _name; }
    [[nodiscard]] std::string getMnemonic() const { return _mnemonic; }

    /**
     * Remove all the links between the divisions.
     */
    void clearLinkedDivisions();

    /**
     * Populate linked divisions from the division names.
     * This method must be called by the engine when all the divisions
     * have been loaded and initialized.
     */
    void populateLinkedDivisions();

    [[nodiscard]] int getLinksCount() const noexcept;
    void enableLink(int i, bool ena);
    [[nodiscard]] bool isLinkEnabled(int i) const;
    Link& getLinkByIndex(int i);
    void cancelAllLinks();


    void clear();
    Stop& addRankwave(Rankwave *ptr, bool ena = false, const std::string& name = std::string());

    void setParamGain(const std::shared_ptr<AudioParameter> &param) noexcept { _paramGain = param; }

    AudioParameterPool& parameters() noexcept { return _params; }

    int getStopsCount() const noexcept;
    void enableStop(int i, bool ena);
    bool isStopEnabled(int i) const;
    Stop& getStopByIndex(int i);
    void disableAllStops();

    void getAvailableRange(int& minNote, int& maxNote) const noexcept;

    int getMIDIChannelsMask() const noexcept { return _midiChannelsMask; }
    bool isForMIDIChannel(int channel) const noexcept;
    void setMIDIChannelsMask(int channelsMask) noexcept { _midiChannelsMask = channelsMask; }

    bool hasSwell() const noexcept { return _hasSwell; }
    void setHasSwell(bool v) noexcept { _hasSwell = v; }
    bool hasTremulant() const noexcept { return _hasTremulant; }
    void setHasTremulant(bool v) noexcept { _hasTremulant = v; }
    bool isTremulantEnabled() const noexcept { return _tremulantEnabled; }
    void setTremulantEnabled(bool ena) noexcept;

    float getTremulantLevel(bool update = true);

    //------------------------------------------------------

    // All the following methods must be called on the audio thread.

    void noteOn(int note, int midiChannel);
    void noteOff(int note, int midiChannel);
    void allNotesOff();

    void handleControlMessage(const MidiMessage& msg);

    bool process(AudioBuffer& targetBuffer, AudioBuffer& voiceBuffer);
    void modulate(AudioBuffer& targetBuffer, const AudioBuffer& tremulantBuffer);

    void releaseVoicesOfDisabledStops();
    void triggerVoicesOfEnabledStops();

    List<Voice>& getActiveVoices() noexcept { return _activeVoices; }

    /**
     * Tells the division has been alreayd triggered by a linked division,
     * so that it should not be receiving the same note on/off event.
     */
    bool hasBeenTriggered() const noexcept { return _triggerFlag; }

    /**
     * Clears trigger flag.
     * This must be called on all divisions before processing the
     * next note on/off event.
     */
    void clearTriggerFlag() noexcept { _triggerFlag = false; }

private:
    /**
     * This will construct the keys aggregated state from the division's keys state
     * and all the coupled from divisions.
     */
    void updateAggregatedKeysState();

    bool triggerVoicesForStop(int stopIndex, int note);

    bool isAlreadyVoiced(int stopIndex, int node);

    std::string _name;     ///< The division name.
    std::string _mnemonic; ///< Short mnemonic name.

    /// List of linked divisions names.
    std::vector<std::string> _linkedDivisionNames{};
    std::vector<Link> _linkedDivisions{};
    std::vector<Division*> _linkedFromDivisions{};

    bool _hasSwell;         ///< Whetehr this division has a swell control.
    bool _hasTremulant;     ///< Whether this division has a remulant control.

    std::atomic<int> _midiChannelsMask;     ///< Division MIDI channels.
    std::atomic<bool> _tremulantEnabled;    ///< Whether tremulant is enabled.

    float _tremulantLevel;
    float _tremulantMaxLevel;
    std::atomic<float> _tremulantTargetLevel;

    /// Stored gain parameter for easy access from the devision control UI component
    std::shared_ptr<AudioParameter> _paramGain;
    AudioParameterPool _params;

    /// Swell low-pass filter.
    dsp::BiquadFilter::Spec _swellFilterSpec;
    dsp::BiquadFilter::State _swellFilterStateL;
    dsp::BiquadFilter::State _swellFilterStateR;

    /// Delay lines used for tremulant frequency modulation.
    dsp::DelayLine _tremulantDelayL;
    dsp::DelayLine _tremulantDelayR;

    std::vector<Stop> _stops{};   ///< All the stops this division has.

    List<Voice> _activeVoices;  ///< Active voices on this division.

    std::bitset<TOTAL_NOTES> _keysState; ///< MIDI keys state 1 = on, 0 = off.
    std::bitset<TOTAL_NOTES> _aggregatedKeysState;   ///< MIDI keys state aggregated from the coupled divisions.

    /// Tells whether this division has been triggered.
    /// This is used to avoid a division to be triggered multiple
    /// times by the same not on/off even, which is the case
    /// for linked divisions.
    bool _triggerFlag;
    const Engine& _engine;

    friend class DivisionFactory;
};


