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
#include "aeolus/voice.h"
#include "aeolus/Division.h"
#include "aeolus/sequencer.h"
#include "aeolus/audioparam.h"
#include "aeolus/dsp/convolver.h"
#include "aeolus/dsp/interpolator.h"
#include "aeolus/MidiManager.h"

#include <optional>
#include <vector>

/**
 * @brief Organ engine.
 * This class defines the top-level organ engine that performs MIDI events processing
 * and audio generation.
 */
class Engine final : public MidiManager
{
    /// Global volume gain.
    constexpr static float VOLUME_GAIN = 0.005f;

    /// Tremulant modulation frequency.
    constexpr static float TREMULANT_FREQUENCY = 6.283184f;
    constexpr static float TREMULANT_PHASE_INCREMENT = std::numbers::pi_v<float> * 2.0f * TREMULANT_FREQUENCY * SAMPLE_RATE_R;

    /// Tremulant OSC wavetable amplitude.
    constexpr static float TREMULANT_LEVEL = 1.0f;

public:
    explicit Engine();
    ~Engine() = default;

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
    GlobalPiston captureStateAsPiston() const;

    auto prepareToPlay() -> void; // Called before starting requesting the audio blocks.
    void setReverbIR(int num); // Set the reverb IR by its number.
    void setReverbWet(float v); // Set reverb wet output level (linear). Audio thread only.
    void setVolume(float v, bool immediate = false); // Set output volume (linear). Audio thread only.

    template<auto OUT_BUFFER_SIZE> // Generate audio. Audio thread only.
    void processNoninterpolatedRealtimeStereo(float (&out)[OUT_BUFFER_SIZE]) {
        ProcessMidiBuffer();
        bool wasAudioGenerated = false;
        memset(out, 0.0f, sizeof(float) * OUT_BUFFER_SIZE);

        constexpr auto STEREO_SUB_FRAME_LENGTH = AUDIO_SUB_FRAME_LENGTH * 2;
        for (int i = 0; i < OUT_BUFFER_SIZE; i += STEREO_SUB_FRAME_LENGTH) {
            generateTremulant();
            for (const auto &division : _divisions) {
                _divisionFrameBuffer.clear();
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
        // When there is no audio generated, we let the reverb tail sound and stop the reverb processing to avoid convolving with silence.
        constexpr auto OUT_PER_CHANNEL_SIZE = OUT_BUFFER_SIZE / OUTPUT_CHANNELS;
        _reverbTailCounter = wasAudioGenerated ? _convolver.length() : std::max(0, _reverbTailCounter - OUT_PER_CHANNEL_SIZE);
        if (_reverbTailCounter > 0 && _convolver.isAudible()) {
            _convolver.process(out, OUT_PER_CHANNEL_SIZE);
        }
        applyVolume(out, OUT_PER_CHANNEL_SIZE);
    }

    [[nodiscard]] std::shared_ptr<VoicePool> getVoicePool() const noexcept { return _voicePool; }
    [[nodiscard]] Division *getDivisionByName(const std::string &name) const;
private:
    void populateDivisions();
    void clearDivisionsTriggerFlag() const;
    bool processSubFrame();
    void generateTremulant(); // Generate tremulant osc waveform for a subframe.
    void applyVolume(AudioBuffer& out); /// Apply the global volume.
    void applyVolume(float* inOut, size_t framesPerChannel);

    std::shared_ptr<VoicePool> _voicePool;
    std::vector<std::unique_ptr<Division>> _divisions{};
    std::vector<GlobalPiston> pistons{};
    std::unique_ptr<Sequencer> _sequencer{};

    StaticAudioBuffer<AUDIO_SUB_FRAME_LENGTH, OUTPUT_CHANNELS> _subFrameBuffer;
    StaticAudioBuffer<AUDIO_SUB_FRAME_LENGTH, OUTPUT_CHANNELS> _divisionFrameBuffer;
    StaticAudioBuffer<AUDIO_SUB_FRAME_LENGTH, OUTPUT_CHANNELS> _voiceFrameBuffer;
    StaticAudioBuffer<AUDIO_SUB_FRAME_LENGTH, 1> _tremulantBuffer;

    AudioParameter _volume;
    float _tremulantPhase;
    dsp::Convolver _convolver;
    std::atomic<int> _selectedIR;
    int _reverbTailCounter;
};


