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
#include "aeolus/globals.h"
#include "aeolus/Division.h"
#include "aeolus/Sequencer.h"
#include "aeolus/dsp/convolver.h"
#include "aeolus/MidiManager.h"
#include "aeolus/VoicePool.h"

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

    void clearDivisionsTriggerFlag() const;

    void generateTremulant(); // Generate tremulant osc waveform for a subframe.

    std::shared_ptr<VoicePool> _voicePool{};
    std::vector<std::unique_ptr<Division>> _divisions{};
    std::vector<GlobalPiston> pistons{};
    std::unique_ptr<Sequencer> _sequencer{};

    StaticAudioBuffer<AUDIO_SUB_FRAME_LENGTH, OUTPUT_CHANNELS> _summingFrameBuffer;
    StaticAudioBuffer<AUDIO_SUB_FRAME_LENGTH, OUTPUT_CHANNELS> _divisionFrameBuffer;
    StaticAudioBuffer<AUDIO_SUB_FRAME_LENGTH, OUTPUT_CHANNELS> _voiceFrameBuffer;
    StaticAudioBuffer<AUDIO_SUB_FRAME_LENGTH, 1> _tremulantBuffer;

    float _tremulantPhase{0.0f};

public:
    explicit Organ(std::function<Rankwave*(const std::string&)> getStopByName);
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

    template<auto OUT_BUFFER_SIZE> // Generate audio. Audio thread only.
    bool process(float (&out)[OUT_BUFFER_SIZE]) {
        bool wasAudioGenerated = false;
        memset(out, 0.0f, sizeof(float) * OUT_BUFFER_SIZE);

        constexpr auto STEREO_SUB_FRAME_LENGTH = AUDIO_SUB_FRAME_LENGTH * 2;
        for (int i = 0; i < OUT_BUFFER_SIZE; i += STEREO_SUB_FRAME_LENGTH) {
            generateTremulant();
            for (const auto &division : _divisions) {
                _divisionFrameBuffer.clear();
                // TODO: Consider per var array in struct for locality and SIMD parallelization.
                const bool hasVoices = division->process(_divisionFrameBuffer, _voiceFrameBuffer);
                wasAudioGenerated |= hasVoices;
                if (hasVoices) {
                    division->modulate(_divisionFrameBuffer, _tremulantBuffer);
                    const auto leftBuffer = _divisionFrameBuffer.getReadPointer(0);
                    const auto rightBuffer = _divisionFrameBuffer.getReadPointer(1);
                    for (int j = 0; j < AUDIO_SUB_FRAME_LENGTH; ++j) {
                        out[j * 2 + i] += leftBuffer[j];
                        out[j * 2 + 1 + i] += rightBuffer[j];
                    }
                }
            }
        }
        return wasAudioGenerated;
    }
};
