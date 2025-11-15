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

#include <memory>
#include "MemoryConstants.h"
#include "aeolus/AddSynth.h"

/**
 * @brief Single pipe wavetable.
 *
 * This class represents a single pipe mapped to a model (additive synth),
 * note, and frequency.
 */
class PipeWave final {
public:
    enum EnvelopeState : uint8_t { OVER, ATTACK, RELEASE };
    struct State {
        std::shared_ptr<PipeWave> pipeWave{};
        EnvelopeState envelopeState{};
        float* position{};
        float interpolationPhase{};
        float interpolationSpeed{};
        float gain{};
        int remainingReleaseFrames{};
        float outputGain{};
        float chiffGain{};

        State() = default;
        void init(const std::shared_ptr<PipeWave>& newPipeWave, const float& newOutputGain, const float& newChiffGain) {
            pipeWave = newPipeWave;
            envelopeState = ATTACK;
            position = pipeWave->attackWaveformStart;
            interpolationPhase = 0.0f;
            interpolationSpeed = 0.0f;
            gain = 1.0f;
            remainingReleaseFrames = pipeWave->releaseFrameCount;
            outputGain = newOutputGain;
            chiffGain = newChiffGain;
        }
    };
    PipeWave(const std::shared_ptr<AddSynth> &model, int note, float freq);

    PipeWave() = delete;
    PipeWave(const PipeWave& other) = delete;
    ~PipeWave() = default;

    [[nodiscard]] int getNote() const noexcept { return note + model->getNoteMin(); }
    [[nodiscard]] float getFreqency() const noexcept { return freq; }
    [[nodiscard]] float getPipeFrequency() const noexcept;

    void generateWavetable();

    void playMono(State &state, std::array<float, PROCESS_FRAMES_SIZE> &out);

private:
    static constexpr auto CENTS_IN_OCTAVE = 1200.0f;
    static constexpr auto AUDIBLE_THRESHOLD = -40.0f;
    static constexpr auto HARMONIC_SKIP_THRESHOLD = -80.0f;
    static constexpr auto DECIBEL_TO_LINEAR_APPROX = 0.1661f;
    static constexpr auto NOISE_SCALING = 0.05f;
    static constexpr auto PLAY_INTERPOLATION_SPEED_SCALING = 0.0005f;
    static void looplen(float fundamentalFreqHz, float effectiveSampleRate, int maxLoopLength, int &optimalLoopLength, int &cycleCount);
    static void attgain(float *att, const int &n, const float &p);

    std::shared_ptr<AddSynth> model;
    int note;
    float freq;

    int attackLength;       // _l0
    int loopLength;         // _l1
    int sampleStep;         // _k_s
    int releaseFrameCount;  // _k_r
    float releaseDecayRate; // _m_r
    float releaseDetune;    // _d_r
    float instability;      // _d_p

    std::vector<float> wavetable;

    float* attackWaveformStart; // _p0
    float* loopWaveformStart;   // _p1
    float* loopEndPtr;          // _p2
};

