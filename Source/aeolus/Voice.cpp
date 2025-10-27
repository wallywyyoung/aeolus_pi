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
#include <utility>
#include "aeolus/Organ.h"


void Voice::init(PipeWave::State newState, const int& newStopIndex) {
    state = std::move(newState);
    stopIndex = newStopIndex;
    spatialSource.init(state.pipeWave->getNote(), static_cast<float>(state.pipeWave->getModel()->getFd()), static_cast<float>(state.pipeWave->getModel()->getFd()));
    const auto freq = state.pipeWave->getPipeFrequency();
    const float att = 1.0f - expf(-freq * FREQUENCY_ROLLOFF);
    chiff.init(freq, 1.0f / freq,
            std::min<float>(1.0f, BASE_CHIFF_INTENSITY * state.chiffGain * att),
            spatialSource.getPostFxSamplesCount() + static_cast<int>(Division::TREMULANT_DELAY_LENGTH));
}

void Voice::release() {
    if (state.envelopeState == PipeWave::Over) {
        std::cerr << "Release was called after the voice was over!" << std::endl;
        return;
    }
    state.release();
    chiff.release();
}

void Voice::process(StaticAudioBuffer<PROCESS_FRAMES_SIZE, OUTPUT_CHANNELS> &out) {
    state.pipeWave->playMono(state, buffer);
    chiff.process(state, buffer);
    spatialSource.process(buffer, out);
}

bool Voice::isOver() const noexcept {
    return state.isOver() && chiff.isOver();
}

bool Voice::isActive() const noexcept {
    return state.envelopeState == PipeWave::Attack;
}

bool Voice::isActiveForStopNote(const int &thisStopIndex, const int &note) const noexcept {
    if (state.pipeWave != nullptr) {
        return isActive() && thisStopIndex == stopIndex && state.pipeWave->getNote() == note;
    }
    return false;
}

int Voice::getNote() const {
    if (state.pipeWave != nullptr) {
        return state.pipeWave->getNote();
    }
    return -1;
}