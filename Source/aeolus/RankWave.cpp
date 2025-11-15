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
#include "Scale.h"
#include "aeolus/AddSynth.h"
#include "aeolus/StaticPipe.h"

RankWave::RankWave(AddSynth model, const Scale& scale, const float tuningFrequency) : noteMin(model.getNoteMinimum()), noteMax(model.getNoteMaximum()), model(std::make_shared<AddSynth>(model)) {
    assert(noteMax - noteMin + 1 > 0);
    staticPipes.clear();
    const auto frequencyNumerator = model.getFrequencyNumerator();
    const auto frequencyDenominator = model.getFrequencyDenominator();
    const auto& s = scale.getTable();
    const float baseFrequency = tuningFrequency;

    for (int i = noteMin; i <= noteMax; ++i) {
        auto frequency = scale.getFrequencyForMidiNote(i, baseFrequency);
        frequency = frequency * static_cast<float>(frequencyNumerator) / static_cast<float>(frequencyDenominator);
        staticPipes.push_back(std::make_shared<StaticPipe>(this->model, i - noteMin, frequency));
    }
}

void RankWave::generateWavetables() {
    for (const auto& staticPipe : staticPipes) {
        staticPipe->generateWavetable();
    }
}

std::shared_ptr<StaticPipe> RankWave::getStaticPipe(const int &note) {
    if (note < noteMin || note > noteMax) {
        return nullptr;
    }
    const int index = note - noteMin;
    assertIsPositiveAndBelow(index, staticPipes.size());

    return staticPipes[index];
}


