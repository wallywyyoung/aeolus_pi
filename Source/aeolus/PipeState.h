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

#include <random>
#include "aeolus/StaticPipe.h"
#include "dsp/Convolution/Convolver.h"

// Try to fit in a single cache line.
class alignas(64) PipeState {
    static constexpr auto PLAY_INTERPOLATION_SPEED_SCALING = 0.0005f;
    static constexpr auto NOISE_SCALING = 0.05f;
    float* loopEnd{}; // Unchanging Data
    float* loopStart{}; // Unchanging Data
    float* position{};
    float interpolationPhase{};
    float interpolationSpeed{};
    float gain{};
    float chiffGain{};
    float instability{}; // Unchanging Data
    float releaseDecayRate{}; // Unchanging Data
    float releaseDetune{}; // Unchanging Data
    uint8_t sampleStep{}; // Unchanging Data
    uint8_t note{}; // Unchanging Data
    uint16_t loopLength{}; // Unchanging Data
    uint16_t remainingReleaseFrames{};
public:
    float outputGain{};
    enum EnvelopeState : uint8_t { OVER, ATTACK, RELEASE };
    EnvelopeState envelopeState{};

    PipeState() = default;

    bool operator==(const PipeState& other) const {
        return loopStart == other.loopStart;
    }

    void init(const StaticPipe* staticPipe, const float& newOutputGain, const float& newChiffGain) {
        envelopeState = ATTACK;
        position = staticPipe->attackStart;
        interpolationPhase = 0.0f;
        interpolationSpeed = 0.0f;
        gain = 1.0f;
        remainingReleaseFrames = staticPipe->releaseFrameCount;
        outputGain = newOutputGain;
        chiffGain = newChiffGain;
        instability = staticPipe->instability;
        releaseDecayRate = staticPipe->releaseDecayRate;
        sampleStep = staticPipe->sampleStep;
        loopLength = staticPipe->loopLength;
        loopEnd = staticPipe->loopEnd;
        loopStart = staticPipe->loopStart;
        releaseDetune = staticPipe->releaseDetune;
        note = staticPipe->note;
    }

    int getNote() const {
        return note;
    }

    void playMono(std::array<float, PROCESS_FRAMES_SIZE> &out) {
        if (envelopeState == OVER) {
            return;
        }
        auto gainDecay = 0.0f;
        if (envelopeState == RELEASE) {
            static constexpr auto PROCESS_FRAMES_SIZE_R = 1.0f / static_cast<float>(PROCESS_FRAMES_SIZE);
            gainDecay = gain * PROCESS_FRAMES_SIZE_R;
            if (remainingReleaseFrames > 0) {
                gainDecay *= releaseDecayRate;
                --remainingReleaseFrames;
            } else {
                envelopeState = OVER;
            }
        }
        if (position < loopStart) {
            for (auto& sample : out) {
                sample = gain * *position;
                ++position;
                gain -= gainDecay;
            }
        } else {
            auto phaseStep = 0.0f;
            if (envelopeState == ATTACK) {
                static std::random_device rnd;
                static std::mt19937 gen(rnd());
                static std::uniform_real_distribution dist(-0.5f, 0.5f);
                interpolationSpeed += instability * PLAY_INTERPOLATION_SPEED_SCALING * (NOISE_SCALING * instability * dist(gen) - interpolationSpeed);
                phaseStep = interpolationSpeed * static_cast<float>(sampleStep);
            } else { // RELEASE
                phaseStep = releaseDetune;
            }
            for (auto& sample : out) {
                interpolationPhase += phaseStep;

                const int phaseOverflow = (interpolationPhase > 1.0f) - (interpolationPhase < 0.0f);
                position += phaseOverflow;
                interpolationPhase -= static_cast<float>(phaseOverflow);

                position += sampleStep;
                if (position >= loopEnd) {
                    position -= loopLength;
                }
                sample = gain * (position [0] + interpolationPhase * (position[1] - position[0]));
                gain -= gainDecay;
            }
        }
    }
};