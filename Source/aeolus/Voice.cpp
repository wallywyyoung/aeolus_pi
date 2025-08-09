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

#include "aeolus/Voice.h"
#include "aeolus/Organ.h"

#include <cstring>

void Voice::trigger(const Pipewave::State& state) {
    assert(_state.isIdle());
    _state = state;
    // Chiff
    const auto freq = _state.pipewave->getPipeFrequency();
    const auto dt = 1.0f / freq;
    // Delay pipe harmonic signal so that chiff noise builds up first
    _delay = static_cast<int>(std::min<float>(SAMPLE_RATE_F, 0.5f * dt * SAMPLE_RATE_F));
    _chiff.setAttack(5.0f * dt);
    _chiff.setDecay(100.0f * dt);
    _chiff.setSustain(0.01f);
    _chiff.setRelease(100.0f * dt);
    // Frequency-dependant chiff attenuation
    const float att = 1.0f - expf(-freq / 3000.0f);
    _chiff.setGain(std::min<float>(1.0f, 0.02f * _state.chiffGain * att));
    _chiff.setFrequency(freq);
    _chiff.trigger();
    // Spatialization
    const int note = _state.pipewave->getNote();
    const float k = note % 2 != 0 ? 1.0f : -1.0f;
    // Wider spread for low-pitched pipes
    const auto& model = _state.pipewave->getModel();
    const float width = 0.15f * static_cast<float>(model->getFd()) / static_cast<float>(model->getFn());
    const float x = width * k * static_cast<float>(abs(note - 65));
    _spatialSource.setSourcePosition(x, 5.0f);
    _spatialSource.recalculate();
    _postReleaseCounter = _spatialSource.getPostFxSamplesCount() + 2 * _delay + static_cast<int>(Division::TREMULANT_DELAY_LENGTH);
}

void Voice::release() {
    // This voice is pending to be reclaimed.
    if (_state.env == Pipewave::Over) {
        return;
    }
    _state.release();
    _chiff.release();
}

void Voice::reset() {
    _state.reset();
    _stopIndex = -1;
    memset(_buffer, 0, sizeof(float) * AUDIO_SUB_FRAME_LENGTH);
    _delayLine.reset();
    _chiff.reset();
    _spatialSource.reset();
}

void Voice::process(float* outL, float* outR) {
    memset(_buffer, 0, sizeof(float) * AUDIO_SUB_FRAME_LENGTH);
    const auto gain = _state.gain;
    if (_state.env == Pipewave::Over) {
        _postReleaseCounter -= std::min(static_cast<int>(_postReleaseCounter), AUDIO_SUB_FRAME_LENGTH);
        for (float & i : _buffer) {
            _delayLine.write(0.0f);
            i = _delayLine.readNearest(_delay) * gain;
        }
    } else {
        auto* pipe = _state.pipewave;
        pipe->play(_state, _buffer);
        for (float & i : _buffer) {
            _delayLine.write(i);
            i = _delayLine.readNearest(_delay) * gain;
        }
    }
    _chiff.process(_buffer, AUDIO_SUB_FRAME_LENGTH);
    // Spatial modeling is only applied on stereo voice output
    if (outL != outR) {
        _spatialSource.process(_buffer, outL, outR, AUDIO_SUB_FRAME_LENGTH);
    } else {
        memcpy(outL, _buffer, sizeof(float) * AUDIO_SUB_FRAME_LENGTH);
    }
}

bool Voice::isOver() const noexcept {
    return (_state.env == Pipewave::Over) && _postReleaseCounter == 0;
}

bool Voice::isActive() const noexcept {
    return _state.env == Pipewave::Attack;
}

bool Voice::isForNote(const int note) const noexcept {
    if (_state.pipewave != nullptr) {
        return _state.pipewave->getNote() == note;
    }
    return false;
}

int Voice::getNote() const {
    if (_state.pipewave != nullptr) {
        return _state.pipewave->getNote();
    }
    return -1;
}

void Voice::resetAndReturnToPool() {
    _resetAndReturn(this);
}
