// ----------------------------------------------------------------------------
//
//  Copyright (C) 2025 Wally Young <wallywyyoung@users.noreply.github.com>
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

//
// #include <atomic>
//
// #include "StaticAudioBuffer.h"
// #include "aeolus/engine.h"
// #include "aeolus/dsp/convolver.h"
//
// struct Organ {
//     enum class StopControlMode {
//         Disabled,   // 0b00
//         SetOff,     // 0b01
//         SetOn,      // 0b10
//         Toggle      // 0b11
//     };
//     enum {
//         VOLUME = 0,
//         NUM_PARAMS
//     };
//
//     std::shared_ptr<AudioParameter> _divisionGain;
//     std::shared_ptr<VoicePool> _voicePool; ///< All the voices.
//     AudioParameterPool _params; ///< Internal parameters.
//     std::optional<StopControlMode> _stopControlMode{};
//     int _stopControlGroup{};
//     int _stopControlButton{};
//     /// List of all divisions
//     std::vector<std::unique_ptr<Division>> _divisions{};
//     std::unique_ptr<Sequencer> _sequencer{};
//     std::vector<int> _sequencerStepBackwardKeySwitches{Engine::SEQUENCER_BACKWARD_MIDI_KEY };
//     std::vector<int> _sequencerStepForwardKeySwitches{Engine::SEQUENCER_FORWARD_MIDI_KEY };
//
//     StaticAudioBuffer<AUDIO_SUB_FRAME_LENGTH, OUTPUT_CHANNELS> _subFrameBuffer;
//     StaticAudioBuffer<AUDIO_SUB_FRAME_LENGTH, OUTPUT_CHANNELS> _divisionFrameBuffer;
//     StaticAudioBuffer<AUDIO_SUB_FRAME_LENGTH, OUTPUT_CHANNELS> _voiceFrameBuffer;
//     StaticAudioBuffer<AUDIO_SUB_FRAME_LENGTH, 1> _tremulantBuffer;
//
//     int _remainedSamples;
//     float _tremulantPhase;
//     dsp::Convolver _convolver;
//     std::atomic<int> _selectedIR;
//     int _reverbTailCounter;
// };
// struct DivisionData {
//     enum Params { GAIN = 0, NUM_PARAMS };
//
//     constexpr static size_t TREMULANT_DELAY_LENGTH = 32; // Frequency modulation delay line length (in samples).
//
//     /// Link with another division.
//     struct Link {
//         Division* division;
//         bool enabled = false;
//     };
//     /// Total number of MIDI notes.
//     constexpr static int TOTAL_NOTES = 128;
//
//     /// Tremulant OSC wavetable amplitude.
//     constexpr static float TREMULANT_TARGET_LEVEL = 0.5f; // Amplitude modulation level.
//     constexpr static float TREMULANT_DELAY_MODULATION_LEVEL = 0.9f; // Frequency modulation level.
//
//     std::string _name;     ///< The division name.
//     std::string _mnemonic; ///< Short mnemonic name.
//
//     /// List of linked divisions names.
//     std::vector<std::string> _linkedDivisionNames{};
//     std::vector<Link> _linkedDivisions{};
//     std::vector<Division*> _linkedFromDivisions{};
//
//     bool _hasSwell;         ///< Whetehr this division has a swell control.
//     bool _hasTremulant;     ///< Whether this division has a remulant control.
//
//     std::atomic<int> _midiChannelsMask;     ///< Division MIDI channels.
//     std::atomic<bool> _tremulantEnabled;    ///< Whether tremulant is enabled.
//
//     float _tremulantLevel;
//     float _tremulantMaxLevel;
//     std::atomic<float> _tremulantTargetLevel;
//
//     /// Stored gain parameter for easy access from the devision control UI component
//     std::shared_ptr<AudioParameter> _paramGain;
//     AudioParameterPool _params;
//
//     /// Swell low-pass filter.
//     dsp::BiquadFilter::Spec _swellFilterSpec;
//     dsp::BiquadFilter::State _swellFilterStateL;
//     dsp::BiquadFilter::State _swellFilterStateR;
//
//     /// Delay lines used for tremulant frequency modulation.
//     dsp::DelayLineStatic<TREMULANT_DELAY_LENGTH> _tremulantDelayL;
//     dsp::DelayLineStatic<TREMULANT_DELAY_LENGTH> _tremulantDelayR;
//
//     std::vector<Stop> _stops{};   ///< All the stops this division has.
//
//     std::vector<Voice*> _activeVoices;  ///< Active voices on this division.
//
//     std::bitset<TOTAL_NOTES> _keysState; ///< MIDI keys state 1 = on, 0 = off.
//     std::bitset<TOTAL_NOTES> _aggregatedKeysState;   ///< MIDI keys state aggregated from the coupled divisions.
//
//     /// Tells whether this division has been triggered.
//     /// This is used to avoid a division to be triggered multiple
//     /// times by the same not on/off even, which is the case
//     /// for linked divisions.
//     bool _triggerFlag;
//     const Engine& _engine;
// };
// struct StopData {
//     // Stop type.
//     enum class Type { Unknown, Principal, Flute, Reed, String };
//
//     // Zone - a grouping pipes for a range of keys.
//     struct Zone {
//         Range keyRange;
//         std::vector<Rankwave *> rankwaves;
//         [[nodiscard]] bool isForKey(const int key) const noexcept { return keyRange.contains(key); }
//     };
//     std::vector<Rankwave *> getRankwavesFromPipeVar(const nlohmann::json &v) const;
//     Type _type{Type::Unknown};
//     std::string _name{};
//     std::vector<Zone> _zones{};
//     float _gain{1.0f};
//     float _chiffGain{0.0f};
//     bool _enabled{false};
// };
// struct RankwaveData {
//     int _noteMin;
//     int _noteMax;
//     std::shared_ptr<Addsynth> model;
//     // Two sets of pipes to be able to switch between tunings
//     // without releasing all the voices.
//     std::vector<std::vector<Pipewave>> _pipes{};
//     std::atomic<int> _pipeSetIndex{ 0 };
// };
// struct PipewaveData {
//     /// Envelope state.
//     enum EnvState
//     {
//         Idle,
//         Attack,
//         Release,
//         Over
//     };
//
//     /// Playback state.
//     struct State
//     {
//         Pipewave *pipewave = nullptr;
//         EnvState env = Idle;
//         float* playPtr = nullptr;               // _p_p
//         float playInterpolation = 0.0;          // _y_p
//         float playInterpolationSpeed = 0.0f;    // _z_p
//         float* releasePtr = nullptr;            // _p_r
//         float releaseInterpolation = 0.0f;      // _y_r
//         float releaseGain = 0.0f;               // _g_r
//         int releaseCount = 0;                   // _i_r
//         float gain = 1.0f;
//         float chiffGain = 0.0f;
//     };
//     std::shared_ptr<Addsynth> _model;
//     int _note;
//     float _freq;
//
//     // Tells whether this pipewave needs to be re-generated.
//     // This is required for example when changing the tuninig.
//     std::shared_ptr<std::atomic<bool>> _needsToBeRebuilt;
//
//     int _attackLength;          // _l0
//     int _loopLength;            // _l1
//     int _sampleStep;            // _k_s
//     int _releaseLength;         // _k_r
//     float _releaseMultiplier;   // _m_r
//     float _releaseDetune;       // _d_r
//     float _instability;         // _d_p
//
//     std::vector<float> _wavetable;
//
//     float* _attackStartPtr; // _p0
//     float* _loopStartPtr;   // _p1
//     float* _loopEndPtr;     // _p2
// };
