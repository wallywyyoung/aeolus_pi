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

#include <chrono>


#include "StaticAudioBuffer.h"
#include "aeolus/Division.h"
#include "aeolus/MidiManager.h"
#include "aeolus/dsp/convolver.h"
#include "aeolus/globals.h"

#include <vector>


/**
 * @brief Organ engine.
 * This class defines the top-level organ engine that performs MIDI events processing
 * and audio generation.
 */
class Organ final : public MidiManager::OrganInterface {
    constexpr static float TREMULANT_FREQUENCY = 6.283184f; /// Tremulant modulation frequency.
    constexpr static float TREMULANT_PHASE_INCREMENT = std::numbers::pi_v<float> * 2.0f * TREMULANT_FREQUENCY * SAMPLE_RATE_R;
    constexpr static float TREMULANT_LEVEL = 1.0f; /// Tremulant OSC wavetable amplitude.

    void generateTremulant(); // Generate tremulant osc waveform for a subframe.

    std::vector<std::shared_ptr<Division>> divisions{};
    std::vector<GlobalPiston> pistons{};

    StaticAudioBuffer<PROCESS_FRAMES_SIZE, OUTPUT_CHANNELS> summingFrameBuffer;
    StaticAudioBuffer<PROCESS_FRAMES_SIZE, OUTPUT_CHANNELS> divisionFrameBuffer;
    StaticAudioBuffer<PROCESS_FRAMES_SIZE, OUTPUT_CHANNELS> voiceFrameBuffer;
    StaticAudioBuffer<PROCESS_FRAMES_SIZE, 1> tremulantFrameBuffer;

    float tremulantPhase{0.0f};

public:
    explicit Organ(const std::function<std::shared_ptr<RankWave>(const std::string&)> &getStopByName);
    ~Organ() override = default;

    // Notes
    void setDivisionNoteOn(const int& division, const int& note) override;
    void setDivisionNoteOff(const int& division, const int& note) override;
    void setDivisionAllNotesOff(const int& division) override;
    void setGlobalAllNotesOff() override;
    // Swell
    void handleDivisionSwell(const int& division, const float& value) override;
    // Stops
    void setDivisionStopOn(const int& division, const int& stop) override;
    void setDivisionStopOff(const int& division, const int& stop) override;
    void setDivisionStopToggle(const int& division, const int& stop) override;
    void setDivisionAllStopsOff(const int& division) override;
    void setDivisionAllStopsOn(const int& division) override;
    void setGlobalAllStopsOff() override;
    void setGlobalAllStopsOn() override;
    // Couplers
    void setDivisionCouplerOn(const int& division, const int& coupler) override;
    void setDivisionCouplerOff(const int& division, const int& coupler) override;
    // Tremulant
    void setDivisionTremulantOn(const int& division) override;
    void setDivisionTremulantOff(const int& division) override;
    // Pistons
    void setDivisionPiston(const int& division, const int& piston) override;
    void recallDivisionPiston(const int& division, const int& piston) override;
    void setGlobalPiston(const int& piston) override;
    void recallGlobalPiston(const int& piston) override;
    void recallGlobalPiston(const GlobalPiston& piston);

    [[nodiscard]] GlobalPiston captureStateAsPiston() const;

    bool process(float (&out)[PROCESS_SAMPLES_SIZE]) {
        auto wasAudioGenerated = false;
        const static auto LEFT_BUFFER = divisionFrameBuffer.getReadPointer(0);
        const static auto RIGHT_BUFFER = divisionFrameBuffer.getReadPointer(1);
            generateTremulant();
            for (const auto &division : divisions) {
                if (!division->process(divisionFrameBuffer, voiceFrameBuffer)) {
                    continue;
                }
                // TODO: MODULATE IS BROKEN
                // division->modulate(divisionFrameBuffer, tremulantFrameBuffer);
                for (auto j = 0; j < PROCESS_FRAMES_SIZE; ++j) {
                    if (wasAudioGenerated) {
                        out[j * 2] += LEFT_BUFFER[j];
                        out[j * 2 + 1] += RIGHT_BUFFER[j];
                    } else {
                        out[j * 2] = LEFT_BUFFER[j];
                        out[j * 2 + 1] = RIGHT_BUFFER[j];
                    }
                }
                wasAudioGenerated = true;
            }
        return wasAudioGenerated;
    }
};
