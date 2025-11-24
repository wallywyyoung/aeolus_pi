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
#include "aeolus/Division.h"

void Voice::init(const std::shared_ptr<StaticPipe>& staticPipe, const float &outputGain, const float &chiffGain, const int &newStopIndex) {
    state.init(staticPipe.get(), outputGain, chiffGain);
    stopIndex = newStopIndex;
    spatialSource.init(staticPipe->getNote(), static_cast<float>(staticPipe->getModel()->getFrequencyDenominator()), static_cast<float>(staticPipe->getModel()->getFrequencyNumerator()));
    const auto freq = staticPipe->getPipeFrequency();
    const float att = 1.0f - expf(-freq * FREQUENCY_ROLLOFF);
    chiff.init(freq, std::min<float>(1.0f, BASE_CHIFF_INTENSITY * chiffGain * att), spatialSource.getPostFxSamplesCount() + static_cast<int>(Division::TREMULANT_DELAY_LENGTH));
}

void Voice::release() {
    if (state.envelopeState == PipeState::OVER) {
        return;
    }
    state.envelopeState = PipeState::RELEASE;
    chiff.release();
}

void Voice::process(StaticAudioBuffer<PROCESS_FRAMES_SIZE, OUTPUT_CHANNELS> &out) {
    state.playMono(buffer);
    chiff.process(state, buffer);
    spatialSource.process(buffer, out);
}

bool Voice::isOver() const noexcept {
    return state.envelopeState == PipeState::OVER;
}

bool Voice::isActive() const noexcept {
    return state.envelopeState == PipeState::ATTACK;
}

bool Voice::isActiveForStopNote(const int &thisStopIndex, const int &note) const noexcept {
    return isActive() && thisStopIndex == stopIndex && state.getNote() == note;
}

int Voice::getNote() const {
    return state.getNote();
}