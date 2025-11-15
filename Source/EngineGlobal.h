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
// ----------------------------------------------------------------------------

#pragma once

#include <memory>
#include <unordered_map>
#include "aeolus/IR.h"
#include "aeolus/MidiManager.h"
#include "aeolus/Model.h"
#include "aeolus/dsp/Convolution/Convolver.h"
#include "aeolus/Scale.h"

/**
 * @brief A global shared instance of the organ engine.
 *
 * This class in a singleton which is shared among all the plugin instances.
 */
class RankWave;
class Organ;

class EngineGlobal final : public MidiManager {
public:
    explicit EngineGlobal();
    ~EngineGlobal() = default;

    void pushMidi(const MidiData& midiData) { push(midiData); }
    void process(float (&out)[PROCESS_SAMPLES_SIZE]);

    std::shared_ptr<Organ> getOrgan() { return organ; }

private:
    constexpr static auto TUNING_FREQUENCY_DEFAULT = 440.0f; /// mid-A tuning frequency.
    void generateWavetables() const;

    Model model;
    std::shared_ptr<Organ> organ;
    std::unordered_map<std::string, std::shared_ptr<RankWave>> rankWavesByName{};
    IRs irs;
    std::shared_ptr<Scale> scale { std::make_shared<Scale>(Scale::EqualTemp)};
    int longestIrLength{};                              ///< Longest IR length in samples
    float tuningFrequency { TUNING_FREQUENCY_DEFAULT }; ///< Middle A tuning frequency.

    Convolver convolver{};
};
