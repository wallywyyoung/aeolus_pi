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

#include "aeolus/globals.h"
#include "aeolus/ringbuffer.h"
#include "aeolus/scale.h"
#include "aeolus/voice.h"
#include "aeolus/addsynth.h"
#include "aeolus/rankwave.h"
#include "aeolus/division.h"
#include "aeolus/sequencer.h"
#include "aeolus/audioparam.h"
#include "aeolus/levelmeter.h"
#include "aeolus/dsp/convolver.h"
#include "aeolus/dsp/interpolator.h"
#include "aeolus/MidiMessage.h"
#include "aeolus/MidiKeyboardState.h"
#include "AudioBuffer.h"

#include <optional>
#include <vector>
#include <any>
#include <set>

AEOLUS_NAMESPACE_BEGIN

class Engine;

/**
 * @brief Organ engine.
 * This class defines the top-level organ engine that performs MIDI events processing
 * and audio generation.
 */
class Engine
{
public:
    struct NoteEvent
    {
        bool on;
        int note;
        int midiChannel;
    };

    struct IRSwithEvent
    {
        int num;
    };

//    struct Level
//    {
//        LevelMeter left;
//        LevelMeter right;
//    };

    enum {
        VOLUME = 0,

        NUM_PARAMS
    };

    //--------------------------------------------------------------------------

    Engine();

    /**
     * This method returns external processing sample rate as mandated
     * by the plugin host. Internally the organ engine performs processing
     * with a fixed SAMPLE_RATE.
     */
    float getSampleRate() const noexcept { return _sampleRate; }

    /**
     * Returns the number of active (playing) voices.
     */
    int getVoiceCount() const noexcept { return _voicePool.getNumberOfActiveVoices(); }

    /**
     * Called by the host pefore starting requesting the audio blocks.
     */
    void prepareToPlay(float sampleRate, int frameSize);

    /**
     * Set the reverb IR bu its number.
     * @note This can be called upon initialisation or on the audio thread.
     */
    void setReverbIR(int num);

    /**
     * Set the reverb IR by its number asynchronously.
     * This to be called on the main (UI) thread.
     */
    void postReverbIR(int num);

    /**
     * Returns currently set reverb IR number.
     */
    int getReverbIR() const noexcept { return _selectedIR; }

    /**
     * Returns the reverb tail in seconds.
     */
    float getReverbLengthInSeconds() const;

    /**
     * Set reverb wet output level (linear).
     * @note This must be called on the audio thread.
     */
    void setReverbWet(float v);

    /**
     * Set global output volume level (linear).
     * @note This must be called on the audio thread.
     */
    void setVolume(float v);

//    /**
//     * Returns volume levels.
//     */
//    Level& getVolumeLevel() noexcept { return _volumeLevel; }

    /**
     * Returns currently set MIDI control channel.
     */
    int getMIDIControlChannelsMask() const noexcept { return _midiControlChannelsMask; }

    /**
     * Assign MIDI channel to be used to control the organ stops and sequencer.
     */
    void setMIDIControlChannelsMask(int mask) noexcept { _midiControlChannelsMask = mask; }

    /**
    * Returns currently set MIDI control channel.
    */
    int getMIDISwellChannelsMask() const noexcept { return _midiSwellChannelsMask; }

    /**
    * Assign MIDI channel to be used to control the organ stops and sequencer.
    */
    void setMIDISwellChannelsMask(int mask) noexcept { _midiSwellChannelsMask = mask; }

    /**
     * Generate audio.
     */
    void process(float* outL, float* outR, int numFrames, bool isNonRealtime = false);

    // Multibus version of the processing (does not include the convolver).
    void process(std::vector<float>& out, bool isNonRealtime = false);

    /**
     * Process incoming MIDI messages.
     */
    void processMIDIMessage(const MidiMessage& message);

    /**
     * Handle note-on events.
     */
    void noteOn(int note, int midiChannel);

    /**
     * Handle note-off events.
     */
    void noteOff(int note, int midiChannel);

    /**
     * Release all the active voices immediately.
     */
    void allNotesOff();

    MidiKeyboardState& getMidiKeyboardState() noexcept { return _midiKeyboardState; }

    Range getMidiKeyboardRange() const;

    std::set<int> getKeySwitches() const;

    VoicePool& getVoicePool() noexcept { return _voicePool; }

    int getDivisionCount() const noexcept { return _divisions.size(); }
    Division* getDivisionByIndex(int i) { return &_divisions[i]; }
    Division* getDivisionByName(const std::string& name);

    Sequencer* getSequencer() noexcept { return _sequencer.get(); }

    std::map<std::string, std::any> getPersistentState() const;
    void setPersistentState(const std::map<std::string, std::any>& state);

    void postNoteEvent(bool onOff, int note, int midiChannel);

private:

//    void populateDivisions();
//    void loadDivisionsFromConfig(std::ifstream& stream);

    void clearDivisionsTriggerFlag();

    bool processSubFrame();

    void processPendingNoteEvents();
    void processPendingIRSwitchEvents();

    /// Generate tremulant osc waveform for a subframe.
    void generateTremulant();

    /// Apply the gloval volume.
    void applyVolume(std::vector<float>& out);
    void applyVolume(float* outL, float* outR, int numFrames);

    /// Process control MIDI messages: program change (sequencer) and stop buttons CC.
    void processControlMIDIMessage(const MidiMessage& message);

    /// Process stop buttons MIDI controls.
    void processStopControlMessage();

    bool isKeySwitchForward(int key) const;
    bool isKeySwitchBackward(int key) const;

    float _sampleRate;

    RingBuffer<NoteEvent, 1024> _pendingNoteEvents;

    VoicePool _voicePool;           ///< All the voices.

    AudioParameterPool _params;     ///< Internal parameters.

    std::optional<StopControlMode> _stopControlMode{};
    int _stopControlGroup{};
    int _stopControlButton{};

    /// List of all divisions
    std::vector<Division> _divisions;

    std::unique_ptr<Sequencer> _sequencer;

    std::vector<int> _sequencerStepBackwardKeySwitches{ SEQUENCER_BACKWARD_MIDI_KEY };
    std::vector<int> _sequencerStepForwardKeySwitches{ SEQUENCER_FORWARD_MIDI_KEY };

    AudioBuffer _subFrameBuffer;
    AudioBuffer _divisionFrameBuffer;
    AudioBuffer _voiceFrameBuffer;

    int _remainedSamples;

    std::vector<float> _tremulantBuffer;
    float _tremulantPhase;

    dsp::Convolver _convolver;
    std::atomic<int> _selectedIR;
    RingBuffer<IRSwithEvent, 1024> _irSwitchEvents;
    int _reverbTailCounter;

    dsp::Interpolator _interpolator;

    MidiKeyboardState _midiKeyboardState;

//    Level _volumeLevel;

    std::atomic<int> _midiControlChannelsMask;
    std::atomic<int> _midiSwellChannelsMask;
};

AEOLUS_NAMESPACE_END
