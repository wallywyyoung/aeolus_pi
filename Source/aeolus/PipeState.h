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
#include "aeolus/StaticPipe.h"

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
    uint16_t loopLength{}; // Unchanging Data
    uint16_t remainingReleaseFrames{};
    uint8_t sampleStep{}; // Unchanging Data
    uint8_t note{}; // Unchanging Data
public:
    enum EnvelopeState : uint8_t { OVER, ATTACK, RELEASE };
    EnvelopeState envelopeState{};
    float outputGain{};

    PipeState() = default;
    bool operator==(const PipeState& other) const;
    void init(const StaticPipe* staticPipe, const float& newOutputGain, const float& newChiffGain);
    [[nodiscard]] int getNote() const;
    void playMono(std::array<float, PROCESS_FRAMES_SIZE> &out);
};
