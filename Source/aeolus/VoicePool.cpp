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

#include "aeolus/VoicePool.h"

VoicePool::VoicePool(Organ& engine, const int maxVoices): _engine{engine}, _voices(maxVoices, Voice(engine)), _voiceCount{0}, _idleVoices(_voices) { }

Voice* VoicePool::trigger(const Pipewave::State& state) {
    if (_idleVoices.size() > 0) {
        auto voice = _idleVoices.begin();
        voice->trigger(state);
        _idleVoices.erase(voice);
        ++_voiceCount;

        return voice;
    }

    // No more voices.
    return nullptr;
}

void VoicePool::resetAndReturnToPool(Voice* voice) {
    assert(voice != nullptr);
    voice->reset();
    _idleVoices.emplace_back(*voice);
    --_voiceCount;
}
