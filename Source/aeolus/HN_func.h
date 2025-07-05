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

#include "aeolus/globals.h"
#include "aeolus/N_func.h"

/**
 * @brief Interpolated per-note look-up table for harmonics.
 *
 * This class keeps a per-note LUT for each of the N_HARM harmonics.
 */
    class HN_func final
    {
    public:
        /// Number of harmonics used.
        constexpr static int N_HARM = 64;

        HN_func();
        void reset(float v);
        void setValue(int idx, float v);            // setv(i, v)
        void setValue(int harm, int idx, float v);  // setv(h, i, v)
        void clearValue(int idx);                   // clrv(i)
        void clearValue(int harm, int idx);         // clrv(h, i);
        [[nodiscard]] float getValue(int harm, int idx) const;    // vs(h, i);
        [[nodiscard]] bool isSet(int harm, int idx) const;        // st(h, i)

        N_func& operator[](int harm) { isPositiveAndBelow(harm, _h.size()); return _h[harm]; }
        const N_func& operator[](int harm) const { isPositiveAndBelow(harm, _h.size()); return _h[harm]; }

        void fromJson(const nlohmann::json& v);

        void read(std::istream& stream, int n = N_HARM);

    private:
        std::array<N_func, N_HARM> _h;
    };

