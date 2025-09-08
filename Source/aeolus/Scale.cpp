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

#include "aeolus/Scale.h"
#include <cmath>

float Scale::getFrequencyForMidiNote(const int midiNote, const float tuningFrequency) const {
    const auto& scaleTable{ getTable() };
    // Base detune from tuning frequency.
    const float baseFrequency{ tuningFrequency / scaleTable[9] };
    // Base note adjusted by detune, then scaled 2^n as octaves.
    return std::ldexp(baseFrequency * scaleTable[midiNote % 12], midiNote / 12 - 5);
}
