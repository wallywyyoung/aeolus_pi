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

void Voice::trigger(const PipeWave::State &newState) {
    assert(state.isIdle());
    state = newState;
    // Chiff
    const auto freq = state.pipeWave->getPipeFrequency();
    const auto dt = 1.0f / freq;
    // Delay pipe harmonic signal so that chiff noise builds up first
    delay = static_cast<int>(std::min<float>(delayLine.size(), 0.5f * dt * SAMPLE_RATE_F));
    chiff.setAttack(5.0f * dt);
    chiff.setDecay(100.0f * dt);
    chiff.setSustain(0.01f);
    chiff.setRelease(100.0f * dt);
    // Frequency-dependant chiff attenuation
    const float att = 1.0f - expf(-freq / 3000.0f);
    chiff.setGain(std::min<float>(1.0f, 0.02f * state.chiffGain * att));
    chiff.setFrequency(freq);
    chiff.trigger();
    // Spatialization
    const int note = state.pipeWave->getNote();
    const float k = note % 2 != 0 ? 1.0f : -1.0f;
    // Wider spread for low-pitched pipes
    const auto& model = state.pipeWave->getModel();
    const float width = 0.15f * static_cast<float>(model->getFd()) / static_cast<float>(model->getFn());
    const float x = width * k * static_cast<float>(abs(note - 65));
    spatialSource.setSourcePosition(x, 5.0f);
    spatialSource.recalculate();
    postReleaseCounter = spatialSource.getPostFxSamplesCount() + 2 * delay + static_cast<int>(Division::TREMULANT_DELAY_LENGTH);
}

void Voice::release() {
    if (state.env == PipeWave::Over) {
        std::cerr << "Release was called before the voice was over!" << std::endl;
        return;
    }
    state.release();
    chiff.release();
}

void Voice::reset() {
    state.reset();
    stopIndex = -1;
    delayLine.reset();
    chiff.reset();
    spatialSource.reset();
}

void Voice::process(StaticAudioBuffer<AUDIO_SUB_FRAME_LENGTH, OUTPUT_CHANNELS> &out) {
    buffer.fill(0.0f);
    const auto gain = state.gain;
    if (state.env == PipeWave::Over) {
        postReleaseCounter -= std::min(static_cast<int>(postReleaseCounter), AUDIO_SUB_FRAME_LENGTH);
        for (float & i : buffer) {
            delayLine.write(0.0f);
            i = delayLine.readNearest(delay) * gain;
        }
    } else {
        const auto pipeWave = state.pipeWave;
        pipeWave->play(state, buffer);
        for (float & i : buffer) {
            delayLine.write(i);
            i = delayLine.readNearest(delay) * gain;
        }
    }
    chiff.process(buffer);
    // spatialSource.process(buffer, out);
    auto l=out.getWritePointer(0);
    auto r=out.getWritePointer(1);
    for (auto i = 0; i < AUDIO_SUB_FRAME_LENGTH; ++i) {
        l[i] = buffer[i];
        r[i] = buffer[i];
    }
}

bool Voice::isOver() const noexcept {
    return (state.env == PipeWave::Over) && postReleaseCounter == 0;
}

bool Voice::isActive() const noexcept {
    return state.env == PipeWave::Attack;
}

bool Voice::isForNote(const int note) const noexcept {
    if (state.pipeWave != nullptr) {
        return state.pipeWave->getNote() == note;
    }
    return false;
}

int Voice::getNote() const {
    if (state.pipeWave != nullptr) {
        return state.pipeWave->getNote();
    }
    return -1;
}