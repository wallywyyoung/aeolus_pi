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
    constexpr static size_t TREMULANT_DELAY_LENGTH = 32; // Frequency modulation delay line length (in samples).

    struct Coupler {
        Division* division;
        bool enabled = false;
    };

    explicit Division(const Engine& engine, const std::string& name = std::string());

    [[nodiscard]] std::string getName() const { return _name; }
    [[nodiscard]] std::string getMnemonic() const { return _mnemonic; }

    void init(); // This method must be called after all divisions have been loaded and initialized.

    // Notes
    void setNoteOn(const int& note, const bool& isLinkedDivision);
    void setNoteOff(const int& note, const bool& updateLinkedDivisions);
    void setAllNotesOff(const bool& isLinkedDivision);
    // Modifiers
    void handleSwell(const int& value);
    // Stops
    void setStopOn(const int& stop);
    void setStopOff(const int& stop);
    void setStopToggle(const int& stop);
    void setAllStopsOff();
    void setAllStopsOn();
    // Couplers
    void setCouplerOn(const int& coupler);
    void setCouplerOff(const int& coupler);
    // Tremulant
    void setTremulantOn();
    void setTremulantOff();
    // Pistons
    void setPiston(const int& piston);
    void recallPiston(const int& piston);
    void recallPiston(const DivisionPiston& piston);
    DivisionPiston captureStateAsPiston() const;

    bool process(StaticAudioBuffer<AUDIO_SUB_FRAME_LENGTH, OUTPUT_CHANNELS>& targetBuffer, StaticAudioBuffer<AUDIO_SUB_FRAME_LENGTH, OUTPUT_CHANNELS>& voiceBuffer);
    void modulate(StaticAudioBuffer<AUDIO_SUB_FRAME_LENGTH, OUTPUT_CHANNELS>& targetBuffer, const StaticAudioBuffer<AUDIO_SUB_FRAME_LENGTH, 1>& tremulantBuffer);

    void releaseVoicesOfDisabledStops();
    void triggerVoicesOfEnabledStops();

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

    void setAllCouplersOff();
    void setAllCouplersOn();

    void updateAggregatedKeysState(); // Aggregates key state from this division's and coulpled divisions' key states.
    bool triggerVoicesForStop(int stopIndex, int note);
    bool isAlreadyVoiced(int stopIndex, int node);

    std::string _name;     ///< The division name.
    std::string _mnemonic; ///< Short mnemonic name.

    /// List of linked divisions names.
    std::vector<std::string> _linkedDivisionNames{};
    std::vector<Coupler> _linkedDivisions{};
    std::vector<Division*> _linkedFromDivisions{};
    std::vector<DivisionPiston> pistons{};

    bool _hasSwell;         ///< Whether this division has a swell control.
    bool _hasTremulant;     ///< Whether this division has a tremulant control.
    std::atomic<bool> _tremulantEnabled;    ///< Whether tremulant is enabled.

    float _tremulantLevel;
    float _tremulantMaxLevel;
    std::atomic<float> _tremulantTargetLevel;

    AudioParameter _paramGain{1};

    /// Swell low-pass filter.
    dsp::BiquadFilter::Spec _swellFilterSpec;
    dsp::BiquadFilter::State _swellFilterStateL;
    dsp::BiquadFilter::State _swellFilterStateR;

    /// Delay lines used for tremulant frequency modulation.
    dsp::DelayLineStatic<TREMULANT_DELAY_LENGTH> _tremulantDelayL;
    dsp::DelayLineStatic<TREMULANT_DELAY_LENGTH> _tremulantDelayR;

    std::vector<Stop> _stops{};   // All the stops this division has.
    std::vector<Voice*> _activeVoices;  // Active voices on this division.

    std::bitset<TOTAL_NOTES> _keysState; // Key state for this division.
    std::bitset<TOTAL_NOTES> _aggregatedKeysState;   // Key state aggregated from coupled divisions.

    /// Tells whether this division has been triggered.
    /// This is used to avoid triggering a division multiple times by the same note on/off event,
    /// which is the case for linked divisions.
    bool _triggerFlag;
    const Engine& _engine;

    friend class DivisionFactory;
};


