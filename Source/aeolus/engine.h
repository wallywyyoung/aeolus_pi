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
#include <set>

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

    /// Number of steps in the sequencer.
    constexpr static int SEQUENCER_N_STEPS = 32;

    constexpr static int SEQUENCER_BACKWARD_MIDI_KEY = 22;
    constexpr static int SEQUENCER_FORWARD_MIDI_KEY = 23;


    enum class StopControlMode {
        Disabled,   // 0b00
        SetOff,     // 0b01
        SetOn,      // 0b10
        Toggle      // 0b11
    };
public:
    enum {
        VOLUME = 0,
        NUM_PARAMS
    };

    std::shared_ptr<AudioParameter> _divisionGain;

    explicit Engine();
    ~Engine() = default;

    // Notes
    void setDivisionNoteOn(const int& division, const int& note) override;
    void setDivisionNoteOff(const int& division, const int& note) override;
    void setDivisionAllNotesOff(const int& division) override;
    void setGlobalAllNotesOff() override;
    // Modifiers
    void handleDivisionSwell(const int& division, const float& value) override;
    void handleDivisionTremulant(const int& division, const float& value) override;
    // Stops
    void setDivisionStopOn(const int& division, const int& stop) override;
    void setDivisionStopOff(const int& division, const int& stop) override;
    void setDivisionStopToggle(const int& division, const int& stop) override;
    void setDivisionAllStopsOff(const int& division) override;
    void setDivisionAllStopsOn(const int& division) override;
    void setGlobalAllStopsOff() override;
    void setGlobalAllStopsOn() override;
    // Pistons
    void setDivisionPiston(const int& division, const int& piston) override;
    void recallDivisionPiston(const int& division, const int& piston) override;
    void setGlobalPiston(const int& piston) override;
    void recallGlobalPiston(const int& piston) override;
    void recallGlobalPiston(const GlobalPiston& piston);
    GlobalPiston captureStateAsPiston() const;

    /**
     * Called by the host pefore starting requesting the audio blocks.
     */
    auto prepareToPlay() -> void;
    /**
     * Set the reverb IR bu its number.
     * @note This can be called upon initialisation or on the audio thread.
     */
    void setReverbIR(int num);
    /**
     * Set reverb wet output level (linear).
     * @note This must be called on the audio thread.
     */
    void setReverbWet(float v);
    /**
     * Set global output volume level (linear).
     * @note This must be called on the audio thread.
     */
    void setVolume(float v, bool immediate = false);

    template<auto OUT_BUFFER_SIZE>
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

    // TODO: Make sequencer work, consider repurposing as choir pedal.
    void handleSequencerSwitch(const int& note);
    std::set<int> getKeySwitches() const;
    [[nodiscard]] Sequencer& getSequencer() const noexcept { return *_sequencer.get(); }

    [[nodiscard]] std::shared_ptr<VoicePool> getVoicePool() const noexcept { return _voicePool; }
    [[nodiscard]] int getDivisionCount() const noexcept { return _divisions.size(); }
    [[nodiscard]] Division *getDivisionByIndex(const int i) { return _divisions[i].get(); }
    [[nodiscard]] Division *getDivisionByName(const std::string &name) const;

private:
    void populateDivisions();
    void clearDivisionsTriggerFlag() const;
    bool processSubFrame();
    /// Generate tremulant osc waveform for a subframe.
    void generateTremulant();
    /// Apply the gloval volume.
    void applyVolume(AudioBuffer& out);
    void applyVolume(float* inOut, size_t framesPerChannel);
    /// Process control MIDI messages: program change (sequencer) and stop buttons CC.
    /// Process stop buttons MIDI controls.
    void processStopControlMessage() const;
    [[nodiscard]] bool isKeySwitchForward(int key) const;
    [[nodiscard]] bool isKeySwitchBackward(int key) const;
    static void populateKeySwitchesVector(std::vector<int>& switches, const nlohmann::json& v);
    std::shared_ptr<VoicePool> _voicePool; ///< All the voices.

    AudioParameterPool _params; ///< Internal parameters.

    std::optional<StopControlMode> _stopControlMode{};
    int _stopControlGroup{};
    int _stopControlButton{};

    /// List of all divisions
    std::vector<std::unique_ptr<Division>> _divisions{};
    std::vector<GlobalPiston> pistons{};
    std::unique_ptr<Sequencer> _sequencer{};

    std::vector<int> _sequencerStepBackwardKeySwitches{ SEQUENCER_BACKWARD_MIDI_KEY };
    std::vector<int> _sequencerStepForwardKeySwitches{ SEQUENCER_FORWARD_MIDI_KEY };

    StaticAudioBuffer<AUDIO_SUB_FRAME_LENGTH, OUTPUT_CHANNELS> _subFrameBuffer;
    StaticAudioBuffer<AUDIO_SUB_FRAME_LENGTH, OUTPUT_CHANNELS> _divisionFrameBuffer;
    StaticAudioBuffer<AUDIO_SUB_FRAME_LENGTH, OUTPUT_CHANNELS> _voiceFrameBuffer;
    StaticAudioBuffer<AUDIO_SUB_FRAME_LENGTH, 1> _tremulantBuffer;

    int _remainedSamples;
    float _tremulantPhase;

    dsp::Convolver _convolver;
    std::atomic<int> _selectedIR;
    int _reverbTailCounter;
};


