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
// ----------------------------------------------------------------------------

#include "aeolus/dsp/SpatialSource.h"
#include "MemoryConstants.h"
#include "StaticAudioBuffer.h"

#include <array>

namespace dsp {

    void SpatialSource::init(const int note, const float fd, const float fn) {
        _sourcePosition.x = STARTING_STEREO_WIDTH * fd / fn * (note % 2 != 0 ? 1.0f : -1.0f) * static_cast<float>(abs(note - MIDDLE_C));
        _sourcePosition.y = PIPE_HEIGHT;
        recalculate();
    }

    void SpatialSource::reset() {
        _delayLine.reset();

        BiquadFilter::resetState(_filterState[0]);
        BiquadFilter::resetState(_filterState[1]);
    }

    void SpatialSource::process(const std::array<float, PROCESS_FRAMES_SIZE> &in, StaticAudioBuffer<PROCESS_FRAMES_SIZE, OUTPUT_CHANNELS> &out) {
        const auto l = out.getWritePointer(0);
        const auto r = out.getWritePointer(1);
        for (auto i = 0; i < in.size(); ++i) {
            _delayLine.write(in[i]);
            l[i] = BiquadFilter::tick(_filterSpec[0], _filterState[0],
                                      _delayLine.readNearest(_leftDelay) * _leftAttenuation);
            r[i] = BiquadFilter::tick(_filterSpec[1], _filterState[1],
                                      _delayLine.readNearest(_rightDelay) * _rightAttenuation);
        }
    }

    static float distanceToCutOffFrequency(const float d) {
        return 22.0e3f * expf(-0.09f * d);
    }

    void SpatialSource::recalculate() {
        static constexpr auto SPEED_OF_SOUND_R = 1.0f / 343.0f; // [m/s] @ 20 deg C

        Position left{-0.5f * _listenerLeftRightDistance, 0.0f};
        Position right{0.5f * _listenerLeftRightDistance, 0.0f};
        left.rotate(_listenerOrientation);
        right.rotate(_listenerOrientation);

        const Position sourceRelativeToListener{_sourcePosition.x - _listenerPosition.x, _sourcePosition.y - _listenerPosition.y};
        const float leftAngle = left.angleTo(sourceRelativeToListener);
        const float rightAngle = right.angleTo(sourceRelativeToListener);

        left.x += _listenerPosition.x;
        left.y += _listenerPosition.y;
        right.x += _listenerPosition.x;
        right.y += _listenerPosition.y;

        const auto leftDistance = _sourcePosition.distanceTo(left);
        const auto rightDistance = _sourcePosition.distanceTo(right);
        const auto maxDistance = std::max(leftDistance, rightDistance);
        const auto maxT = maxDistance * SPEED_OF_SOUND_R;
        const auto delayLengthInSamples = static_cast<size_t>(std::lround(SAMPLE_RATE_F * maxT));

        _delayLine.resize(delayLengthInSamples);

        _leftDelay = static_cast<int>(roundf(leftDistance * SAMPLE_RATE_F * SPEED_OF_SOUND_R));
        _rightDelay = static_cast<int>(roundf(rightDistance * SAMPLE_RATE_F * SPEED_OF_SOUND_R));

        constexpr auto att = 0.7f; // [0..1]

        // Angular attenuation
        _leftAttenuation = 0.5f * att * (std::cosf(leftAngle) + 1.0f) + 1.0f - att;
        _rightAttenuation = 0.5f * att * (std::cosf(rightAngle) + 1.0f) + 1.0f - att;

        _filterSpec[0].type = BiquadFilter::LowPass;
        _filterSpec[0].dbGain = 0.0f;
        _filterSpec[0].q = 0.7071f;

        _filterSpec[1] = _filterSpec[0];

        _filterSpec[0].freq = distanceToCutOffFrequency(leftDistance);
        _filterSpec[1].freq = distanceToCutOffFrequency(rightDistance);

        BiquadFilter::updateSpec(_filterSpec[0]);
        BiquadFilter::updateSpec(_filterSpec[1]);
    }

} // namespace dsp

