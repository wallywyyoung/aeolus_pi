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

#include "aeolus/globals.h"

//==============================================================================

namespace math {

float exp2ap(float x)
{
    int i = static_cast<int>(floor(x));
    x -= i;
    // return ldexp (1 + x * (0.66 + 0.34 * x), i);
    return ldexp(1 + x * (0.6930f + x * (0.2416f + x * (0.0517f + x * 0.0137f))), i);
}

} // namespace math

//==============================================================================

namespace midi {

int channelToMask(int channel)
{
    // Zero means any MIDI channel.
    if (channel <= 0)
        return (1 << 16) - 1;

    return 1 << (channel - 1);
}

bool matchChannelToMask(int mask, int channel)
{
    return (mask & channelToMask(channel)) != 0;
}

} // namespace midi


