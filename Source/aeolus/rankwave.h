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

#pragma once

#include "aeolus/globals.h"
#include "aeolus/addsynth.h"
#include "aeolus/scale.h"
#include "aeolus/Pipewave.h"

#include <vector>
#include <atomic>

AEOLUS_NAMESPACE_BEGIN

/**
 * @brief Pipes across the keys range.
 *
 * This class is a collection of pipes based on the same
 * additive synth model.
 */
class Rankwave
{
public:
    explicit Rankwave(std::shared_ptr<Addsynth> model);

    void createPipes(const Scale& scale, float tuningFreq);

    // Recalculate pipes tuning based on the current global scale and A4 frequency,
    // or global MTS tuning if enabed.
    void retunePipes(const Scale& scale, float tuningFreq);

    std::string getStopName() const { return _model->getStopName(); }
    bool isForNote(int note) const noexcept { return note >= _noteMin && note <= _noteMax; }
    int getNoteMin() const noexcept { return _noteMin; }
    int getNoteMax() const noexcept { return _noteMax; }

    void prepareToPlay(float sampleRate);

    Pipewave::State trigger(int note);

private:
    std::shared_ptr<Addsynth> _model;
    int _noteMin;
    int _noteMax;

    // Two sets of pipes to be able to switch between tunings
    // without releasing all the voices.
    std::vector<std::vector<Pipewave>> _pipes;
    std::atomic<int> _pipeSetIndex{ 0 };
};

AEOLUS_NAMESPACE_END
