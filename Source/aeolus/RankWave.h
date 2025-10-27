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

#pragma once

#include "aeolus/Addsynth.h"
#include "aeolus/PipeWave.h"
#include "aeolus/Scale.h"

#include <vector>

/**
 * @brief Pipes across the key range.
 *
 * This class is a collection of pipes based on the same
 * additive synth model.
 */
class RankWave {
public:
    explicit RankWave(Addsynth model, const Scale& scale, float tuningFreq);
    RankWave(const RankWave&) = delete;
    RankWave& operator=(const RankWave& other) = delete;

    [[nodiscard]] const std::string& getStopName() const { return model->getStopName(); }
    [[nodiscard]] bool isForNote(const int note) const noexcept { return note >= noteMin && note <= noteMax; }
    [[nodiscard]] int getNoteMin() const noexcept { return noteMin; }
    [[nodiscard]] int getNoteMax() const noexcept { return noteMax; }

    void generateWavetables();

    std::shared_ptr<PipeWave> getPipeWave(const int &note);

private:
    void createPipes(const Scale& scale, float tuningFrequency);
    int noteMin;
    int noteMax;
    std::shared_ptr<Addsynth> model;
    std::vector<std::shared_ptr<PipeWave>> pipeWaves{};
};


