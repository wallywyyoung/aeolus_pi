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

HN_func::HN_func() : _h{} { }

void HN_func::reset(float v)
{
    for (auto& h : _h) {
        h.reset(v);
    }
}

void HN_func::setValue(const int idx, const float v)
{
    isPositiveAndBelow(idx, N_NOTES);

    for (auto& h: _h) {
        h.setValue(idx, v);
    }
}

void HN_func::setValue(const int harm, const int idx, const float v)
{
    isPositiveAndBelow(harm, _h.size());
    isPositiveAndBelow(idx, N_NOTES);

    _h[harm].setValue(idx, v);
}

void HN_func::clearValue(const int idx)
{
    isPositiveAndBelow(idx, N_NOTES);

    for (auto& h : _h) {
        h.clearValue(idx);
    }
}

void HN_func::clearValue(const int harm, const int idx)
{
    isPositiveAndBelow(harm, _h.size());
    isPositiveAndBelow(idx, N_NOTES);

    _h[harm].clearValue(idx);
}

float HN_func::getValue(const int harm, const int idx) const
{
    isPositiveAndBelow(harm, _h.size());
    isPositiveAndBelow(idx, N_NOTES);

    return _h[harm].getValue(idx);
}

bool HN_func::isSet(const int harm, const int idx) const
{
    isPositiveAndBelow(harm, _h.size());
    isPositiveAndBelow(idx, N_NOTES);

    return _h[harm].isSet(idx);
}

void HN_func::fromJson(const nlohmann::json& v)
{
    if (v.size() >= _h.size()) {
        for (int i = 0; i < _h.size(); ++i)
            _h[i].fromJson(v[i]);
    }
}

void HN_func::read(std::istream& stream, int n)
{
    const auto m = std::min(_h.size(), static_cast<size_t>(n));

    for (int i = 0; i < m; ++i)
        _h[i].read(stream);
}
