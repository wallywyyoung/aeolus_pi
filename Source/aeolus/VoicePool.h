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
// ---------------------------------------------------------------------------

#pragma once

#include "aeolus/Voice.h"

/**
 * @brief A collection of all the voices.
 */
class VoicePool final {
public:

    VoicePool() = default;

    [[nodiscard]] Voice* trigger(const PipeWave::State& state) {
        for (auto& voice : voices) {
            if (voice.isIdle()) {
                voice.trigger(state);
                return &voice;
            }
        }
        // No more voices.
        return nullptr;
    }

private:
    constexpr static int MAX_VOICES = 512;
    std::array<Voice, MAX_VOICES> voices{}; ///< Voices available to be triggered.
};
