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
#include "aeolus/rankwave.h"

Rankwave::Rankwave(Addsynth model, const Scale& scale, const float tuningFreq) : _noteMin(model.getNoteMin()), _noteMax(model.getNoteMax()), model(std::make_shared<Addsynth>(model)), _pipes{2} {
    assert(_noteMax - _noteMin + 1 > 0);
    createPipes(scale, tuningFreq);
}

Rankwave::Rankwave(const Rankwave& other) : _noteMin(other._noteMin), _noteMax(other._noteMax), model(other.model), _pipes{2} {

}

auto Rankwave::createPipes(const Scale &scale, const float tuningFrequency) -> void {
    for (auto& p : _pipes)
        p.clear();

    _pipeSetIndex = 0;

    const auto fn = model->getFn();
    const auto fd = model->getFd();
    const auto& s = scale.getTable();
    float fbase = tuningFrequency * static_cast<float>(fn) / static_cast<float>(fd);

    for (int i = _noteMin; i <= _noteMax; ++i) {
        for (size_t j = 0; j < _pipes.size(); ++j) {
            auto pipe = Pipewave(model, i - _noteMin, scale.getFrequencyForMidiNote(i, fbase));
            _pipes[j].push_back(std::move(pipe));
        }
    }
}

void Rankwave::retunePipes(const Scale& scale, const float tuningFrequency)
{
    const float fnd = static_cast<float>(model->getFn()) / static_cast<float>(model->getFd());

    const int pipeSetIndex{ _pipeSetIndex.load() };
    const int nextPipeSetIndex{ (pipeSetIndex + 1) % static_cast<int>(_pipes.size()) };

    if (EngineGlobal::getInstance().isMTSEnabled()) {
        // Use MTS provided tuning
        for (int i = _noteMin; i <= _noteMax; ++i) {
            auto& pipe = _pipes[nextPipeSetIndex].at(i - _noteMin);

            // @note MTS tuning may return some weird frequencies, we need to clamp them
            if (const float f{limitRange(0.1f, SAMPLE_RATE * 0.5f - 0.1f, EngineGlobal::getInstance().getMTSNoteToFrequency(i, -1) * fnd) }; pipe.getPipeFrequency() != f) {
                pipe.setFrequency(f);
                pipe.setNeedsToBeRebuilt(true);
            }
        }
    } else {
        // Use local scale
        const float fbase = tuningFrequency * fnd;

        for (int i = _noteMin; i <= _noteMax; ++i) {
            auto& pipe = _pipes[nextPipeSetIndex][i - _noteMin];
            pipe.setFrequency(scale.getFrequencyForMidiNote(i, fbase));
            pipe.setNeedsToBeRebuilt(true);
        }
    }
}

void Rankwave::prepareToPlay(const float sampleRate)
{
    const int pipeSetIndex{ _pipeSetIndex.load() };
    const int nextPipeSetIndex{ (pipeSetIndex + 1) % static_cast<int>(_pipes.size()) };

    for (auto& pipe : _pipes[nextPipeSetIndex]) {
        pipe.prepateToPlay(sampleRate);
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


