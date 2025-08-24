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

#include "aeolus/RankWave.h"
#include "EngineGlobal.h"

RankWave::RankWave(Addsynth model, const Scale& scale, const float tuningFreq) : noteMin(model.getNoteMin()), noteMax(model.getNoteMax()), model(std::make_shared<Addsynth>(model)) {
    assert(noteMax - noteMin + 1 > 0);
    createPipes(scale, tuningFreq);
}

auto RankWave::createPipes(const Scale &scale, const float tuningFrequency) -> void {
    pipeWaves.clear();
    const auto fn = model->getFn();
    const auto fd = model->getFd();
    const auto& s = scale.getTable();
    const float fbase = tuningFrequency * static_cast<float>(fn) / static_cast<float>(fd);

    for (int i = noteMin; i <= noteMax; ++i) {
        pipeWaves.push_back(std::make_shared<PipeWave>(model, i - noteMin, scale.getFrequencyForMidiNote(i, fbase)));
    }
}

void RankWave::generateWavetables() {
    for (const auto & pipeWave : pipeWaves) {
        pipeWave->generateWavetable();
    }
}

PipeWave::State RankWave::trigger(const int &note, const float &outputGain, const float &chiffGain) {
    if (note < noteMin || note > noteMax) {
        return PipeWave::State(nullptr, 0.0f, 0.0f);
    }

    const int index = note - noteMin;
    assertIsPositiveAndBelow(index, pipeWaves.size());

    return PipeWave::State(pipeWaves[index], outputGain, chiffGain);
}


