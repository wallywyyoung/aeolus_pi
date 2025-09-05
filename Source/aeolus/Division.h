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

#include "StaticAudioBuffer.h"
#include "aeolus/AudioParameter.h"
#include "aeolus/Stop.h"
#include "aeolus/Voice.h"
#include "aeolus/dsp/filter.h"

#include <bitset>
#include <vector>

class Organ;
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
        std::shared_ptr<Division> division;
        bool enabled = false;
    };

    explicit Division(const std::string& name = std::string());

    [[nodiscard]] std::string getName() const { return name; }
    [[nodiscard]] std::string getMnemonic() const { return mnemonic; }

    // Notes
    void setNoteOn(const int &note);
    void setNoteOff(const int &note);
    void setAllNotesOff();
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
    [[nodiscard]] DivisionPiston captureStateAsPiston() const;

    bool process(StaticAudioBuffer<PROCESS_FRAMES_SIZE, OUTPUT_CHANNELS> &divisionBuffer, StaticAudioBuffer<PROCESS_FRAMES_SIZE, OUTPUT_CHANNELS> &voiceBuffer);
    void modulate(StaticAudioBuffer<PROCESS_FRAMES_SIZE, OUTPUT_CHANNELS>& targetBuffer, const StaticAudioBuffer<PROCESS_FRAMES_SIZE, 1>& tremulantBuffer);

    void releaseVoicesOfDisabledStops();
    void triggerVoicesOfEnabledStops();

private:
    constexpr static int TOTAL_NOTES = 128; ///< Total number of MIDI notes.
    /// Tremulant OSC wavetable amplitude.
    constexpr static float TREMULANT_TARGET_LEVEL = 0.5f; ///< Amplitude modulation level.
    constexpr static float TREMULANT_DELAY_MODULATION_LEVEL = 0.9f; ///< Frequency modulation level.

    void setAllCouplersOff();
    void setAllCouplersOn();

    void recursiveKeyState(std::bitset<TOTAL_NOTES> &aggregated, std::vector<Division *> &traversed);
    bool triggerVoicesForStop(int stopIndex, int note);
    bool isAlreadyVoiced(int stopIndex, int note);

    std::string name;     ///< The division name.
    std::string mnemonic; ///< Short mnemonic name.

    std::vector<std::string> linkedDivisionNames{};              ///< List of linked divisions names.
    std::vector<std::shared_ptr<Coupler>> linkedDivisions{};     ///< List of divisions here links to.
    std::vector<std::shared_ptr<Coupler>> linkedFromDivisions{}; ///< List of divisions that link to here.
    std::vector<DivisionPiston> pistons{};

    bool hasSwell;         ///< Whether this division has a swell control.
    bool hasTremulant;     ///< Whether this division has a tremulant control.
    bool tremulantEnabled; ///< Whether tremulant is enabled.


    AudioParameter tremulantLevel {0.0f, 0.0f, TREMULANT_TARGET_LEVEL, 0.1f};
    AudioParameter gain{ 1.0f };

    /// Swell low-pass filter.
    dsp::BiquadFilter::Spec swellFilterSpec;
    dsp::BiquadFilter::State swellFilterStateL;
    dsp::BiquadFilter::State swellFilterStateR;

    /// Delay lines used for tremulant frequency modulation.
    dsp::DelayLineStatic<TREMULANT_DELAY_LENGTH> tremulantDelayL;
    dsp::DelayLineStatic<TREMULANT_DELAY_LENGTH> tremulantDelayR;

    std::vector<Stop> stops{};            ///< All the stops this division has.
    std::vector<Voice> activeVoices;     ///< Active voices on this division.

    std::bitset<TOTAL_NOTES> keysState;           ///< Key state for this division.
    std::bitset<TOTAL_NOTES> aggregatedKeysState; ///< Key state aggregated from coupled divisions.

    friend class DivisionFactory;
};


