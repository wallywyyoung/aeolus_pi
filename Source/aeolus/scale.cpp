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

#include "aeolus/scale.h"

#include <cmath>
#include <stdexcept>


const Scale::Map Scale::_scales = {
    {
        Pythagorean,
        {
            1.00000000f,
            1.06787109f,
            1.12500000f,
            1.18518519f,
            1.26562500f,
            1.33333333f,
            1.42382812f,
            1.50000000f,
            1.60180664f,
            1.68750000f,
            1.77777778f,
            1.89843750f,
        }
    },
    {
        MeanQuart,
        {
            1.0000000f,
            1.0449067f,
            1.1180340f,
            1.1962790f,
            1.2500000f,
            1.3374806f,
            1.3975425f,
            1.4953488f,
            1.5625000f,
            1.6718508f,
            1.7888544f,
            1.8691860f,
        }
    },
    {
        Werckm3,
        {
            1.00000000f,
            1.05349794f,
            1.11740331f,
            1.18518519f,
            1.25282725f,
            1.33333333f,
            1.40466392f,
            1.49492696f,
            1.58024691f,
            1.67043633f,
            1.77777778f,
            1.87924088f,
        }
    },
    {
        Kirnberg3,
        {
            1.00000000f,
            1.05349794f,
            1.11848107f,
            1.18518519f,
            1.25000021f,
            1.33333333f,
            1.40625000f,
            1.49542183f,
            1.58024691f,
            1.67176840f,
            1.77777778f,
            1.87500000f,
        }
    },
    {
        WellTemp,
        {
            1.00000000f,
            1.05468828f,
            1.12246205f,
            1.18652432f,
            1.25282725f,
            1.33483985f,
            1.40606829f,
            1.49830708f,
            1.58203242f,
            1.67705161f,
            1.77978647f,
            1.87711994f,
        }
    },
    {
        EqualTemp,
        {
            1.00000000f,
            1.05946309f,
            1.12246205f,
            1.18920712f,
            1.25992105f,
            1.33483985f,
            1.41421356f,
            1.49830708f,
            1.58740105f,
            1.68179283f,
            1.78179744f,
            1.88774863f,
        }
    },
    {
        Ahrend,
        {
            1.00000000f,
            1.05064661f,
            1.11891853f,
            1.18518519f,
            1.25197868f,
            1.33695184f,
            1.40086215f,
            1.49594019f,
            1.57596992f,
            1.67383521f,
            1.78260246f,
            1.87288523f,
        }
    },
    {
        Vallotti,
        {
            1.00000000f,
            1.05647631f,
            1.12035146f,
            1.18808855f,
            1.25518740f,
            1.33609659f,
            1.40890022f,
            1.49689777f,
            1.58441623f,
            1.67705160f,
            1.78179744f,
            1.87888722f,
        }
    },
    {
        Kellner,
        {
            1.00000000f,
            1.05349794f,
            1.11891853f,
            1.18518519f,
            1.25197868f,
            1.33333333f,
            1.40466392f,
            1.49594019f,
            1.58024691f,
            1.67383521f,
            1.77777778f,
            1.87796802f,
        }
    },
    {
        Lehman,
        {
            1.00000000f,
            1.05826737f,
            1.11992982f,
            1.18786496f,
            1.25424281f,
            1.33634808f,
            1.41102316f,
            1.49661606f,
            1.58560949f,
            1.67610496f,
            1.77978647f,
            1.88136421f,
        }
    },
    {
        Pure,
        {
            1.00000000f,
            1.04166667f,
            1.12500000f,
            1.18920000f,
            1.25000000f,
            1.33333333f,
            1.40625000f,
            1.50000000f,
            1.58740000f,
            1.66666667f,
            1.77777778f,
            1.87500000f,
        }
    }
};

Scale::Scale(const Type type)
    : _type(type)
{
}

const Scale::Table& Scale::getTable() const
{
    const auto it = _scales.find(_type);
    if (it == _scales.end()) {
        throw std::runtime_error("Scale type not found");
    }
    return it->second;
}

float Scale::getFrequencyForMidiNote(const int midiNote, const float tuningFrequency) const
{
    const auto& scaleTable{ getTable() };
    const float fbase{ tuningFrequency / scaleTable[9] };
    return ldexp(fbase * scaleTable[midiNote % 12], midiNote / 12 - 5);
}

std::string Scale::getNameForType(const Type type)
{
    static const std::map<Type, std::string> names {
        { Pythagorean, "Pythagorean" },
        { MeanQuart,   "Meantone" },
        { Werckm3,     "Werckmeister" },
        { Kirnberg3,   "Kirnberger" },
        { WellTemp,    "Well tempered" },
        { EqualTemp,   "Equal tempered" },
        { Ahrend,      "Ahrend" },
        { Vallotti,    "Vallotti" },
        { Kellner,     "Kellner" },
        { Lehman,      "Lehman" },
        { Pure,        "Just intonation / C" }
    };

    if (const auto it = names.find(type); it != names.end())
        return it->second;

    return {};
}


