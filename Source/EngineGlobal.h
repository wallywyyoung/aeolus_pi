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

class EngineGlobal final  : public MidiManager {
public:
    EngineGlobal();
    ~EngineGlobal() = default;

    [[nodiscard]] std::shared_ptr<RankWave> getStopByName(const std::string &name) const { return _rankwavesByName.at(name); }
    void pushMidi(const MidiData& midiData) { push(midiData); }

    void process(float (&out)[PROCESS_SAMPLES_SIZE]) {
        // for (int i = 0; i < PROCESS_SAMPLES_SIZE; ++i) {
        //     auto time = static_cast<float>(i) * SAMPLE_RATE_R;
        //     auto val = sinf(2.0f * std::numbers::pi_v<float> * 110.0f * time);;
        //     out[i * 2 + 1] = val;
        //     out[i * 2 + 0] = val;
        // }
        // return;
        // Midi / Configuration Block
        ProcessMidiBuffer();
        // Organ Block
        bool wasAudioGenerated = organ->process(out);
        // Reverb Block
        // When there is no audio generated, we let the reverb tail sound and stop the reverb processing to avoid convolving with silence.
        _reverbTailCounter = wasAudioGenerated ? _convolver.length() : std::max(0, _reverbTailCounter - PROCESS_FRAMES_SIZE);
        if (_reverbTailCounter > 0 && _convolver.isAudible()) {
            _convolver.process(out, PROCESS_FRAMES_SIZE);
        }
        // Volume Block
        // if (_volume.isSmoothing()) {
        //     for (int i = 0; i < OUT_PER_CHANNEL_SIZE; ++i) {
        //         const float g = _volume.nextValue();
        //         out[i*2] *= g;
        //         out[i*2+1] *= g;
        //     }
        // } else {
        //     const float g = _volume.target();
        //     for (int i = 0; i < OUT_PER_CHANNEL_SIZE; ++i) {
        //         out[i*2] *= g;
        //         out[i*2+1] *= g;
        //     }
        // }
    }

private:
    constexpr static float TUNING_FREQUENCY_DEFAULT = 440.0f; /// mid-A tuning frequency.

    void loadRankwaves();
    void generateWavetables() const;

    Model model;
    Organ *organ;
    std::unordered_map<std::string, std::shared_ptr<RankWave>> _rankwavesByName{};
    IRs irs;
    std::shared_ptr<Scale> _scale;
    int _longestIRLength{};   ///< Longest IR length in samples
    float _tuningFrequency;

    // AudioParameter _volume;
    dsp::Convolver _convolver;
    int _reverbTailCounter{0};
};
