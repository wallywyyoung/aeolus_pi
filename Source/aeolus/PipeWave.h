// ----------------------------------------------------------------------------
//
//  Copyright (C) 2025 Wally Young <wallywyyoung@users.noreply.github.com>
//  Copyright (C) 2021 Arthur Benilov <arthur.benilov@gmail.com>
//  Copyright (C) 2003-2013 Fons Adriaensen <fons@linuxaudio.org>
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
// ---------------------------------------------------------------------------

#pragma once

#include "MemoryConstants.h"
#include "aeolus/Addsynth.h"

#include <iostream>
#include <memory>


/**
 * @brief Single pipe wavetable.
 *
 * This class represents a single pipe mapped to a model (additive synth),
 * note, and frequency.
 */
class PipeWave final {
public:
    enum EnvelopeState { Idle, Attack, Release, Over }; /// Envelope state.

    /// Playback state.
    struct State {
        std::shared_ptr<PipeWave> pipeWave{nullptr};
        EnvelopeState envelopeState{Idle};
        float* playbackPosition{nullptr};      // _p_p
        float playInterpolationPhase{0.0f};    // _y_p
        float playInterpolationSpeed{0.0f};    // _z_p

        float* releasePosition{nullptr};       // _p_r
        float releaseInterpolationPhase{0.0f}; // _y_r
        float releaseGain{0.0f};               // _g_r
        int remainingReleaseFrames{0};         // _i_r

        float outputGain;
        float chiffGain;

        explicit State(std::shared_ptr<PipeWave> pipeWave, const float& outputGain, const float& chiffGain) : pipeWave(pipeWave), envelopeState(Attack), outputGain(outputGain), chiffGain(chiffGain) {}

        void release() { envelopeState = Release; }
        [[nodiscard]] bool isTriggered() const noexcept { return pipeWave != nullptr && envelopeState == Attack; }
        [[nodiscard]] bool isIdle() const noexcept { return envelopeState == Idle; }
        [[nodiscard]] bool isOver() const noexcept { return envelopeState == Over; }
    };

    PipeWave() = delete;
    PipeWave(const std::shared_ptr<Addsynth> &model, int note, float freq);
    PipeWave(const PipeWave& other) = delete;
    ~PipeWave() = default;


    [[nodiscard]] std::shared_ptr<Addsynth> getModel() const noexcept { return _model; }

    [[nodiscard]] int getNote() const noexcept { return _note + _model->getNoteMin(); }
    [[nodiscard]] float getFreqency() const noexcept { return _freq; }
    [[nodiscard]] float getPipeFrequency() const noexcept;

    void generateWavetable();

    void play(State &state, std::array<float, PROCESS_FRAMES_SIZE> &out);

private:
    static constexpr auto CENTS_IN_OCTAVE = 1200.0f;
    static constexpr auto AUDIBLE_THRESHOLD = -40.0f;
    static constexpr auto HARMONIC_SKIP_THRESHOLD = -80.0f;
    static constexpr auto DECIBEL_TO_LINEAR_APPROX = 0.1661f;
    static constexpr auto NOISE_SCALING = 0.05f;
    static constexpr auto PLAY_INTERPOLATION_SPEED_SCALING = 0.0005f;
    static void looplen(float fundamentalFreqHz, float effectiveSampleRate, int maxLoopLength, int &optimalLoopLength,
                        int &cycleCount);
    static void attgain(float* att, int n, float p);

    std::shared_ptr<Addsynth> _model;
    int _note;
    float _freq;

    int _attackLength;          // _l0
    int _loopLength;            // _l1
    int _sampleStep;            // _k_s
    int releaseFrameCount;     // _k_r
    float releaseDecayRate;     // _m_r
    float _releaseDetune;       // _d_r
    float _instability;         // _d_p

    std::vector<float> _wavetable;

    float* attackWaveformStart; // _p0
    float* loopWaveformStart;   // _p1
    float* _loopEndPtr;         // _p2
};

