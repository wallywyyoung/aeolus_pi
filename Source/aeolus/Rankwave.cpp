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

#include "aeolus/EngineGlobal.h"
#include "aeolus/Rankwave.h"

Rankwave::Rankwave(Addsynth model, const Scale& scale, const float tuningFreq) : _noteMin(model.getNoteMin()), _noteMax(model.getNoteMax()), model(std::make_shared<Addsynth>(model)), _pipes{2} {
    assert(_noteMax - _noteMin + 1 > 0);
    createPipes(scale, tuningFreq);
}

Rankwave::Rankwave(const Rankwave& other) : _noteMin(other._noteMin), _noteMax(other._noteMax), model(other.model), _pipes{2}, _pipeSetIndex(other._pipeSetIndex.load()) {

}

Rankwave & Rankwave::operator=(const Rankwave &other) {
    if (this != &other) {
        _noteMin = other._noteMin;
        _noteMax = other._noteMax;
        model = other.model;
        _pipes = other._pipes;
        _pipeSetIndex = other._pipeSetIndex.load();
    }
    return *this;
}

auto Rankwave::createPipes(const Scale &scale, const float tuningFrequency) -> void {
    for (auto& p : _pipes)
        p.clear();

    _pipeSetIndex = 0;

    const auto fn = model->getFn();
    const auto fd = model->getFd();
    const auto& s = scale.getTable();
    const float fbase = tuningFrequency * static_cast<float>(fn) / static_cast<float>(fd);

    for (int i = _noteMin; i <= _noteMax; ++i) {
        for (size_t j = 0; j < _pipes.size(); ++j) {
            auto pipe = Pipewave(model, i - _noteMin, scale.getFrequencyForMidiNote(i, fbase));
            _pipes[j].push_back(std::move(pipe));
        }
    }
}

void Rankwave::retunePipes(const Scale& scale, const float tuningFrequency) {
    const float fnd = static_cast<float>(model->getFn()) / static_cast<float>(model->getFd());

    const int pipeSetIndex{ _pipeSetIndex.load() };
    const int nextPipeSetIndex{ (pipeSetIndex + 1) % static_cast<int>(_pipes.size()) };

    // Use local scale
    const float fbase = tuningFrequency * fnd;

    for (int i = _noteMin; i <= _noteMax; ++i) {
        auto& pipe = _pipes[nextPipeSetIndex][i - _noteMin];
        pipe.setFrequency(scale.getFrequencyForMidiNote(i, fbase));
        pipe.setNeedsToBeRebuilt(true);
    }
}

void Rankwave::prepareToPlay()
{
    const int pipeSetIndex{ _pipeSetIndex.load() };
    const int nextPipeSetIndex{ (pipeSetIndex + 1) % static_cast<int>(_pipes.size()) };

    for (auto& pipe : _pipes[nextPipeSetIndex]) {
        pipe.prepareToPlay();
    }

    _pipeSetIndex.store(nextPipeSetIndex);
}

Pipewave::State Rankwave::trigger(const int note)
{
    if (note < _noteMin || note > _noteMax)
        return {};

    const int index = note - _noteMin;

    const int pipeSetIndex{ _pipeSetIndex.load() };

    isPositiveAndBelow(index, _pipes[pipeSetIndex].size());

    auto &pipe = _pipes[pipeSetIndex][note - _noteMin];
    return pipe.trigger();
}


