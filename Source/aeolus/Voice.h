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
// ---------------------------------------------------------------------------

#pragma once

#include "aeolus/PipeState.h"
#include "aeolus/dsp/Chiff.h"
#include "aeolus/dsp/SpatialSource.h"

/**
 * @brief Single voice associated with a single pipe.
 */
class Voice {
    constexpr static auto FREQUENCY_ROLLOFF = 1.0f / 3000.0f;
    constexpr static auto MIDDLE_C = 65;
    constexpr static auto BASE_CHIFF_INTENSITY = 0.02f;
    constexpr static auto STARTING_STEREO_WIDTH = 0.15f;

    PipeState state{}; ///< Pipe state associated with this voice.
    int stopIndex{-1}; /// Index of the stop associated with this voice. This is used to tell which stops are voiced.
    std::array<float, PROCESS_FRAMES_SIZE> buffer{ 0.0f };
    dsp::SpatialSource spatialSource{}; /// Stereo spatial modeller.
    dsp::Chiff chiff{}; /// Attack chiff.
public:
    Voice() = default;
    void init(const std::shared_ptr<StaticPipe>& staticPipe, const float &outputGain, const float &chiffGain, const int &newStopIndex);
    void release();
    void process(StaticAudioBuffer<PROCESS_FRAMES_SIZE, OUTPUT_CHANNELS> &out);

    [[nodiscard]] bool isOver() const noexcept;
    [[nodiscard]] bool isActive() const noexcept;
    [[nodiscard]] bool isActiveForStopNote(const int &thisStopIndex, const int &note) const noexcept;
    [[nodiscard]] int getNote() const;
    [[nodiscard]] int getStopIndex() const noexcept { return stopIndex; }
};
