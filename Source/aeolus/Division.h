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

#include "aeolus/rankwave.h"
#include "aeolus/stop.h"
#include "aeolus/voice.h"
#include "aeolus/audioparam.h"
#include "aeolus/dsp/filter.h"
#include "StaticAudioBuffer.h"

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
class Division {

public:
    enum Params { GAIN = 0, NUM_PARAMS };

    constexpr static size_t TREMULANT_DELAY_LENGTH = 32; // Frequency modulation delay line length (in samples).

    /// Link with another division.
    struct Coupler {
        Division* division;
        bool enabled = false;
    };

    explicit Division(const Engine& engine, const std::string& name = std::string());

    const Engine& getEngine() const noexcept { return _engine; }

    [[nodiscard]] std::string getName() const { return _name; }
    [[nodiscard]] std::string getMnemonic() const { return _mnemonic; }

    /**
     * Remove all the links between the divisions.
     */
    void clearCouplers();

    /**
     * Populate linked divisions from the division names.
     * This method must be called by the engine when all the divisions
     * have been loaded and initialized.
     */
    void populateCouplers();

    [[nodiscard]] int getCouplerCount() const noexcept;
    [[nodiscard]] bool isCouplerEnabled(const int &coupler) const;
    [[nodiscard]] Coupler& getCouplerByIndex(const int &coupler);
    void enableCoupler(const int &coupler, const bool &enabled);
    void cancelAllCouplers();

    void clear();
    Stop& addRankwave(Rankwave *ptr, const bool &ena = false, const std::string& name = std::string());

    void setParamGain(const std::shared_ptr<AudioParameter> &param) noexcept { _paramGain = param; }

    AudioParameterPool& parameters() noexcept { return _params; }

    int getStopsCount() const noexcept;
    bool isStopEnabled(const int &i) const;
    Stop& getStopByIndex(const int &i);

    void getAvailableRange(int& minNote, int& maxNote) const noexcept;

    int getMIDIChannelsMask() const noexcept { return _midiChannelsMask; }
    bool isForMIDIChannel(const int &channel) const noexcept;
    void setMIDIChannelsMask(const int channelsMask) noexcept { _midiChannelsMask = channelsMask; }

    bool hasSwell() const noexcept { return _hasSwell; }
    void setHasSwell(const bool& v) noexcept { _hasSwell = v; }
    bool hasTremulant() const noexcept { return _hasTremulant; }
    void setHasTremulant(const bool& v) noexcept { _hasTremulant = v; }
    bool isTremulantEnabled() const noexcept { return _tremulantEnabled; }
    void setTremulantEnabled(const bool& ena) noexcept;

    float getTremulantLevel(const bool &update = true);

    //------------------------------------------------------

    // All the following methods must be called on the audio thread.
    // Notes
    void setNoteOn(const int& note, const bool& isLinkedDivision);
    void setNoteOff(const int& note, const bool& updateLinkedDivisions);
    void setAllNotesOff(const bool& isLinkedDivision);
    // Modifiers
    void handleSwell(const int& value);
    void handleTremulant(const float& value);
    // Stops
    void setStopOn(const int& stop);
    void setStopOff(const int& stop);
    void setStopToggle(const int& stop);
    void setAllStopsOff();
    void setAllStopsOn();
    DivisionPiston captureStateAsPiston() const;
    // Pistons
    void setPiston(const int& piston);
    void recallPiston(const int& piston);
    void recallPiston(const DivisionPiston& piston);

    bool process(StaticAudioBuffer<AUDIO_SUB_FRAME_LENGTH, OUTPUT_CHANNELS>& targetBuffer, StaticAudioBuffer<AUDIO_SUB_FRAME_LENGTH, OUTPUT_CHANNELS>& voiceBuffer);
    void modulate(StaticAudioBuffer<AUDIO_SUB_FRAME_LENGTH, OUTPUT_CHANNELS>& targetBuffer, const StaticAudioBuffer<AUDIO_SUB_FRAME_LENGTH, 1>& tremulantBuffer);

    void releaseVoicesOfDisabledStops();
    void triggerVoicesOfEnabledStops();

    std::vector<Voice *> &getActiveVoices() noexcept { return _activeVoices; }

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
    /// Total number of MIDI notes.
    constexpr static int TOTAL_NOTES = 128;

    /// Tremulant OSC wavetable amplitude.
    constexpr static float TREMULANT_TARGET_LEVEL = 0.5f; // Amplitude modulation level.
    constexpr static float TREMULANT_DELAY_MODULATION_LEVEL = 0.9f; // Frequency modulation level.

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
    std::vector<Coupler> _linkedDivisions{};
    std::vector<Division*> _linkedFromDivisions{};
    std::vector<DivisionPiston> pistons{};

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
    dsp::DelayLineStatic<TREMULANT_DELAY_LENGTH> _tremulantDelayL;
    dsp::DelayLineStatic<TREMULANT_DELAY_LENGTH> _tremulantDelayR;

    std::vector<Stop> _stops{};   ///< All the stops this division has.

    std::vector<Voice*> _activeVoices;  ///< Active voices on this division.

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


