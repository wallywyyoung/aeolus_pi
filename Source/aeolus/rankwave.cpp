// ----------------------------------------------------------------------------
//
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
// ----------------------------------------------------------------------------

#include "rankwave.h"
#include "EngineGlobal.h"
#include <cstring>

AEOLUS_NAMESPACE_BEGIN

Rankwave::Rankwave(std::shared_ptr<Addsynth> model)
    : _model(model)
    , _noteMin(model->getNoteMin())
    , _noteMax(model->getNoteMax())
    , _pipes{2}
{
    assert(_noteMax - _noteMin + 1 > 0);
}

void Rankwave::createPipes(const Scale& scale, float tuningFrequency) {
    for (auto& p : _pipes)
        p.clear();

    _pipeSetIndex = 0;

    const auto fn = _model->getFn();
    const auto fd = _model->getFd();
    const auto& s = scale.getTable();
    float fbase = tuningFrequency * static_cast<float>(fn) / static_cast<float>(fd);

    for (int i = _noteMin; i <= _noteMax; ++i) {
        for (size_t j = 0; j < _pipes.size(); ++j) {
            auto pipe = Pipewave(_model, i - _noteMin, scale.getFrequencyForMidiNote(i, fbase));
            _pipes[j].push_back(std::move(pipe));
        }
    }
}

void Rankwave::retunePipes(const Scale& scale, float tuningFrequency)
{
    const float fnd = (float)_model->getFn() / (float)_model->getFd();

    int pipeSetIndex{ _pipeSetIndex.load() };
    int nextPipeSetIndex{ (pipeSetIndex + 1) % (int)_pipes.size() };

    if (EngineGlobal::getInstance().isMTSEnabled()) {
        // Use MTS provided tuning
        for (int i = _noteMin; i <= _noteMax; ++i) {
            auto& pipe = _pipes[nextPipeSetIndex].at(i - _noteMin);

            // @note MTS tuning may return some weird frequencies, we need to clamp them
            const float f{limitRange(0.1f, SAMPLE_RATE * 0.5f - 0.1f, EngineGlobal::getInstance().getMTSNoteToFrequency(i) * fnd) };

            if (pipe.getPipeFrequency() != f) {
                pipe.setFrequency(f);
                pipe.setNeedsToBeRebuilt(true);
            }
        }
    } else {
        // Use local scale
        float fbase = tuningFrequency * fnd;

        for (int i = _noteMin; i <= _noteMax; ++i) {
            auto& pipe = _pipes[nextPipeSetIndex][i - _noteMin];
            pipe.setFrequency(scale.getFrequencyForMidiNote(i, fbase));
            pipe.setNeedsToBeRebuilt(true);
        }
    }
}

void Rankwave::prepareToPlay(float sampleRate)
{
    int pipeSetIndex{ _pipeSetIndex.load() };
    int nextPipeSetIndex{ (pipeSetIndex + 1) % (int)_pipes.size() };

    for (auto& pipe : _pipes[nextPipeSetIndex]) {
        pipe.prepateToPlay(sampleRate);
    }

    _pipeSetIndex.store(nextPipeSetIndex);
}

Pipewave::State Rankwave::trigger(int note)
{
    if (note < _noteMin || note > _noteMax)
        return {};

    const int index = note - _noteMin;

    int pipeSetIndex{ _pipeSetIndex.load() };

    if (!isPositiveAndBelow(index, _pipes[pipeSetIndex].size())) {
        assert(false);
        return {};
    }

    auto &pipe = _pipes[pipeSetIndex][note - _noteMin];
    return pipe.trigger();
}

AEOLUS_NAMESPACE_END
