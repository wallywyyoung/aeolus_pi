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

#include "aeolus/Rankwave.h"
#include "aeolus/dsp/chiff.h"
#include "aeolus/dsp/delay.h"
#include "aeolus/dsp/spatial.h"

class Organ;
/**
 * @brief Single voice associated with a single pipe.
 */
class Voice {
public:
    Voice() = delete;
    explicit Voice(Organ& engine);

    Voice& operator=(const Voice& other) {
        if (this != &other) {
            // TODO: Fix assignment.
            // _engine = EngineGlobal::getInstance()->getEngine();
            _state = other._state;
            _stopIndex = other._stopIndex;
            for (int i = 0; i < AUDIO_SUB_FRAME_LENGTH; ++i) {
                _buffer[i] = other._buffer[i];
            }
            _delayLine = other._delayLine;
            _delay = other._delay;
            _chiff = other._chiff;
            _spatialSource = other._spatialSource;
            _postReleaseCounter = other._postReleaseCounter;
        }
        return *this;
    }
    void trigger(const Pipewave::State& state);
    void release();
    void reset();
    void process(float* outL, float* outR);
    void setStopIndex(const int idx) noexcept { _stopIndex = idx; }
    void resetAndReturnToPool();

    [[nodiscard]] bool isOver() const noexcept;
    [[nodiscard]] bool isActive() const noexcept;
    [[nodiscard]] bool isForNote(int note) const noexcept;
    [[nodiscard]] int getNote() const;
    [[nodiscard]] int stopIndex() const noexcept { return _stopIndex; }

private:
    Organ& _engine;
    Pipewave::State _state; ///< Pipe state associated with this voice.
    int _stopIndex{-1}; /// Index of the stop associated with this voice. This is used to tell which stops are voiced.
    float _buffer[AUDIO_SUB_FRAME_LENGTH]{};
    dsp::DelayLineStatic<SAMPLE_RATE> _delayLine; /// Delay after chiff.
    int _delay{};
    dsp::Chiff _chiff; /// Attack chiff.
    dsp::SpatialSource _spatialSource{}; /// Stereo spatial modeller.
    size_t _postReleaseCounter{0}; /// Counter to account for the delayed sound before recycling the voice.
};
