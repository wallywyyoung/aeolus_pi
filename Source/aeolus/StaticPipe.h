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
#include "aeolus/AddSynth.h"

/**
 * @brief Single pipe wavetable.
 *
 * This class represents a single pipe mapped to a model (additive synth),
 * note, and frequency.
 */
class StaticPipe final {
public:
    StaticPipe() = delete;
    StaticPipe(const std::shared_ptr<AddSynth> &model, int note, float freq);
    StaticPipe(const StaticPipe& other) = delete;
    ~StaticPipe() = default;

    [[nodiscard]] std::shared_ptr<AddSynth> getModel() const noexcept { return model; }
    [[nodiscard]] int getNote() const noexcept { return note + model->getNoteMinimum(); }
    [[nodiscard]] float getPipeFrequency() const noexcept;

    void generateWavetable();

private:
    static constexpr auto CENTS_IN_OCTAVE = 1200.0f;
    static constexpr auto AUDIBLE_THRESHOLD = -40.0f;
    static constexpr auto HARMONIC_SKIP_THRESHOLD = -80.0f;
    static constexpr auto DECIBEL_TO_LINEAR_APPROX = 0.1661f;
    static void calculateLoopLength(float fundamentalFreqHz, float effectiveSampleRate, int maxLoopLength, int &optimalLoopLength, int &cycleCount);
    static void attgain(float *att, const int &n, const float &p);
    static float exp2ap(float x);
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
    float* attackStart;     // _p0
    float* loopStart;       // _p1
    float* loopEnd;         // _p2

    friend class PipeState;
};

