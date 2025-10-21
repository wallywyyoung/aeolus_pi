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

#include <unordered_map>

#include "aeolus/Model.h"
#include "aeolus/Organ.h"
#include "aeolus/RankWave.h"
#include "aeolus/Scale.h"

/**
 * @brief A global shared instance of the organ engine.
 *
 * This class in a singleton which is shared among all the plugin instances.
 */

class EngineGlobal final : public MidiManager {
public:
    explicit EngineGlobal();
    ~EngineGlobal() = default;

    [[nodiscard]] std::shared_ptr<RankWave> getStopByName(const std::string &name) const { return _rankwavesByName.at(name); }
    void pushMidi(const MidiData& midiData) { push(midiData); }

    void process(float (&out)[PROCESS_SAMPLES_SIZE]) {
        // Midi / Configuration Block
        ProcessMidiBuffer();
        // Organ Block
        const bool wasAudioGenerated = organ->process(out);
        // Reverb Block
        convolver.process<PROCESS_FRAMES_SIZE>(out, wasAudioGenerated);
    }

private:
    constexpr static auto TUNING_FREQUENCY_DEFAULT = 440.0f; /// mid-A tuning frequency.
    void generateWavetables() const;

    Model model;
    Organ *organ;
    std::unordered_map<std::string, std::shared_ptr<RankWave>> _rankwavesByName{};
    IRs irs;
    std::shared_ptr<Scale> scale { std::make_shared<Scale>(Scale::EqualTemp)};
    int longestIrLength{};                              ///< Longest IR length in samples
    float tuningFrequency { TUNING_FREQUENCY_DEFAULT }; ///< Middle A tuning frequency.

    Convolver convolver{};
};
