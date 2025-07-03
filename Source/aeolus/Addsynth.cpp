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
// ----------------------------------------------------------------------------

#include "aeolus/Addsynth.h"
#include <fstream>
#include <cassert>
#include <map>
#include <cstring>

template <typename T>
static inline bool isPositiveAndBelow(T valueToTest, T upperLimit) {
    return valueToTest >= 0 && valueToTest < upperLimit;
}

void Addsynth::reset()
{
    _noteMin = NOTE_MIN;
    _noteMax = NOTE_MAX;
    _fn = 1;
    _fd = 1;

    _n_vol.reset(-20.0f);
    _n_ins.reset(0.0f);
    _n_off.reset(0.0f);
    _n_att.reset(0.01f);
    _n_atd.reset(0.0f);
    _n_dct.reset(0.01f);
    _n_dcd.reset(0.0f);
    _n_ran.reset(0.0f);
    _h_lev.reset(-100.0f);
    _h_ran.reset(0.0f);
    _h_att.reset(0.050f);
    _h_atp.reset(0.0f);
}
