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

#include "aeolus/RankWave.h"
#include "aeolus/dsp/chiff.h"
#include "aeolus/dsp/delay.h"
#include "aeolus/dsp/spatial.h"

#include <functional>

/**
 * @brief Single voice associated with a single pipe.
 */
class Voice {
    PipeWave::State state{}; ///< Pipe state associated with this voice.
    int stopIndex{-1}; /// Index of the stop associated with this voice. This is used to tell which stops are voiced.
    std::array<float, AUDIO_SUB_FRAME_LENGTH> buffer{ 0.0f };
    dsp::DelayLineStatic<SAMPLE_RATE> delayLine{}; /// Delay after chiff.
    int delay{};
    dsp::Chiff chiff{}; /// Attack chiff.
    dsp::SpatialSource spatialSource{}; /// Stereo spatial modeller.
    size_t postReleaseCounter{0}; /// Counter to account for the delayed sound before recycling the voice.
public:
    Voice() = default;

    Voice& operator=(const Voice& other) = delete;

    void trigger(const PipeWave::State &newState);
    void release();
    void reset();
    void process(StaticAudioBuffer<AUDIO_SUB_FRAME_LENGTH, OUTPUT_CHANNELS> &out);
    void setStopIndex(const int idx) noexcept { stopIndex = idx; }

    [[nodiscard]] bool isOver() const noexcept;
    [[nodiscard]] bool isIdle() const noexcept { return state.env == PipeWave::Idle; }
    [[nodiscard]] bool isActive() const noexcept;
    [[nodiscard]] bool isForNote(int note) const noexcept;
    [[nodiscard]] int getNote() const;
    [[nodiscard]] int getStopIndex() const noexcept { return stopIndex; }
};
