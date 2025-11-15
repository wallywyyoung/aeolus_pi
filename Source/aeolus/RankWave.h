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

#include <memory>
#include <vector>

class AddSynth;
class Scale;
class StaticPipe;
/**
 * @brief Pipes across the key range.
 *
 * This class is a collection of pipes based on the same
 * additive synth model.
 */
class RankWave {
public:
    explicit RankWave(AddSynth model, const Scale& scale, float tuningFrequency);
    RankWave(const RankWave&) = delete;
    RankWave& operator=(const RankWave& other) = delete;

    [[nodiscard]] int getNoteMin() const noexcept { return noteMin; }
    [[nodiscard]] int getNoteMax() const noexcept { return noteMax; }

    void generateWavetables();
    [[nodiscard]] std::shared_ptr<StaticPipe> getStaticPipe(const int &note);

private:
    int noteMin;
    int noteMax;
    std::shared_ptr<AddSynth> model;
    std::vector<std::shared_ptr<StaticPipe>> staticPipes{};
};


