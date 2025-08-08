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
    constexpr static int DefaultMaxVoices = 512;

    explicit VoicePool(Organ& engine, int maxVoices = DefaultMaxVoices);

    [[nodiscard]] int getNumberOfActiveVoices() const noexcept { return _voiceCount; }
    [[nodiscard]] Voice* trigger(const Pipewave::State& state);
    void resetAndReturnToPool(Voice* voice);

private:
    Organ& _engine;
    std::vector<Voice> _voices{}; ///< All the voices.
    std::vector<Voice> _idleVoices; ///< Voices available to be triggered.
    std::atomic<int> _voiceCount; ///< Number of taken voices.
};
