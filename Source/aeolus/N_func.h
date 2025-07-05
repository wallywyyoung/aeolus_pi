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

#pragma once

#include <array>
#include <nlohmann/json.hpp>

/**
 * @brief Interpolated per-note look-up table.
 *
 * This class stores a float parameter across the
 * N_NOTES points. Notes in between get interpolated linearly.
 */

class N_func final
{
public:
    /// Number of notes used in parameters look-up table.
    constexpr static int N_NOTES = 11;

    N_func();
    void reset(float v);
    void setValue(int idx, float v);    // setv(i, v)
    void clearValue(int idx);           // clrv(i)
    float getValue(int idx) const;      // vs(i)
    bool isSet(int idx) const;          // st(i)

    /// Returns interpolated value for a note number (starting from 0).
    float operator[](int note) const;   // vi(n)
    void fromJson(const nlohmann::json& v);
    void read(std::istream& stream);

private:
    /// Gap between the N_NOTES notes within the look-up tables.
    constexpr static int NOTES_GAP = 6;

    int _b;
    std::array<float, N_NOTES> _v;
};


