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
    constexpr static float VOLUME_GAIN = 1.0f;

    /// Tremulant modulation frequency.
    constexpr static float TREMULANT_FREQUENCY = 6.283184f;
    constexpr static float TREMULANT_PHASE_INCREMENT = static_cast<float>(M_PI) * 2.0f * TREMULANT_FREQUENCY / SAMPLE_RATE;

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

    /**
     * This method returns external processing sample rate as mandated
     * by the plugin host. Internally the organ engine performs processing
     * with a fixed SAMPLE_RATE.
     */
    [[nodiscard]] float getSampleRate() const noexcept { return _sampleRate; }

    /**
     * Returns the number of active (playing) voices.
     */
    [[nodiscard]] int getVoiceCount() const noexcept { return _voicePool->getNumberOfActiveVoices(); }

    /**
     * Called by the host pefore starting requesting the audio blocks.
     */
    auto prepareToPlay(float sampleRate) -> void;

    /**
     * Set the reverb IR bu its number.
     * @note This can be called upon initialisation or on the audio thread.
     */
    void setReverbIR(int num);

    /**
     * Returns currently set reverb IR number.
     */
    [[nodiscard]] int getReverbIR() const noexcept { return _selectedIR; }

    /**
     * Returns the reverb tail in seconds.
     */
    [[nodiscard]] float getReverbLengthInSeconds() const;

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
#if AEOLUS_MULTIBUS_OUTPUT
    /**
     * Multibus version of the processing (does not include the convolver).
     */
    void process(AudioBuffer &out);
#else
    /**
     * Generate audio.
     */
    void process(float* outL, float* outR, size_t numFrames, bool isNonRealtime = false);

    size_t processNoninterpolatedRealtime(float *outL, float *outR, size_t numFrames);
#endif
    /**
     * Process incoming MIDI messages.
     */

    void allStopsOn();
    void handleNoteOn(const int &channel, const int &note) override;
    void handleNoteOff(const int &channel, const int &note) override;
    void handleAllNotesOff() override;
    void handleCC(const int& channel, const int& cc, const int& value) override;
    void handlePC(const int& pc) override;
    void handleSequencerSwitch(const int& note) override;

    // MidiManager& getMidiKeyboardState() noexcept { return _midiKeyboardState; }

    [[nodiscard]] Range getMidiKeyboardRange() const;

    [[nodiscard]] std::set<int> getKeySwitches() const;

    [[nodiscard]] std::shared_ptr<VoicePool> getVoicePool() const noexcept { return _voicePool; }

    [[nodiscard]] int getDivisionCount() const noexcept { return _divisions.size(); }
    Division *getDivisionByIndex(const int i) { return _divisions[i].get(); }

    [[nodiscard]] Division *getDivisionByName(const std::string &name) const;

    [[nodiscard]] Sequencer& getSequencer() const noexcept { return *_sequencer.get(); }

private:
    void populateDivisions();
    void clearDivisionsTriggerFlag() const;
    bool processSubFrame();
    /// Generate tremulant osc waveform for a subframe.
    void generateTremulant();
    /// Apply the gloval volume.
    void applyVolume(AudioBuffer& out);
    void applyVolume(float* outL, float* outR, size_t numFrames);
    /// Process control MIDI messages: program change (sequencer) and stop buttons CC.
    /// Process stop buttons MIDI controls.
    void processStopControlMessage() const;
    [[nodiscard]] bool isKeySwitchForward(int key) const;
    [[nodiscard]] bool isKeySwitchBackward(int key) const;
    static void populateKeySwitchesVector(std::vector<int>& switches, const nlohmann::json& v);

    float _sampleRate;

    std::shared_ptr<VoicePool> _voicePool;           ///< All the voices.

    AudioParameterPool _params;     ///< Internal parameters.

    std::optional<StopControlMode> _stopControlMode{};
    int _stopControlGroup{};
    int _stopControlButton{};

    /// List of all divisions
    std::vector<std::unique_ptr<Division>> _divisions{};

    std::unique_ptr<Sequencer> _sequencer{};

    std::vector<int> _sequencerStepBackwardKeySwitches{ SEQUENCER_BACKWARD_MIDI_KEY };
    std::vector<int> _sequencerStepForwardKeySwitches{ SEQUENCER_FORWARD_MIDI_KEY };

    StaticAudioBuffer<SUB_FRAME_LENGTH, N_OUTPUT_CHANNELS> _subFrameBuffer;
    StaticAudioBuffer<SUB_FRAME_LENGTH, N_OUTPUT_CHANNELS> _divisionFrameBuffer;
    StaticAudioBuffer<SUB_FRAME_LENGTH, N_OUTPUT_CHANNELS> _voiceFrameBuffer;
    StaticAudioBuffer<SUB_FRAME_LENGTH, 1> _tremulantBuffer;

    int _remainedSamples;
    float _tremulantPhase;

    dsp::Convolver _convolver;
    std::atomic<int> _selectedIR;
    int _reverbTailCounter;

    dsp::Interpolator _interpolator;

    // MidiManager _midiKeyboardState;
};


