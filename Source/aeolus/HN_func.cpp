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

#include "aeolus/HN_func.h"

HN_func::HN_func(const float& v) {
    const auto vn = N_func(v);
    _h.fill(vn);
}

void HN_func::setValue(const int idx, const float v)
{
    assertIsPositiveAndBelow(idx, N_func::N_NOTES);

    for (auto& h: _h) {
        h.setValue(idx, v);
    }
}

void HN_func::setValue(const int harm, const int idx, const float v)
{
    assertIsPositiveAndBelow(harm, _h.size());
    assertIsPositiveAndBelow(idx, N_func::N_NOTES);

    _h[harm].setValue(idx, v);
}

void HN_func::clearValue(const int idx)
{
    assertIsPositiveAndBelow(idx, N_func::N_NOTES);

    for (auto& h : _h) {
        h.clearValue(idx);
    }
}

void HN_func::clearValue(const int harm, const int idx)
{
    assertIsPositiveAndBelow(harm, _h.size());
    assertIsPositiveAndBelow(idx, N_func::N_NOTES);

    _h[harm].clearValue(idx);
}

float HN_func::getValue(const int harm, const int idx) const
{
    assertIsPositiveAndBelow(harm, _h.size());
    assertIsPositiveAndBelow(idx, N_func::N_NOTES);

    return _h[harm].getValue(idx);
}

bool HN_func::isSet(const int harm, const int idx) const
{
    assertIsPositiveAndBelow(harm, _h.size());
    assertIsPositiveAndBelow(idx, N_func::N_NOTES);

    return _h[harm].isSet(idx);
}