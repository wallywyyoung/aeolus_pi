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

#include "aeolus/N_func.h"
#include "aeolus/globals.h"

N_func::N_func()
        : _b{}
        , _v{}
{
    reset(0.0f);
}

void N_func::reset(const float v)
{
    _b = 16;
    _v.fill(v);
}

void N_func::setValue(const int idx, const float v)
{
    if (! isPositiveAndBelow(idx, N_NOTES))
        return;

    _v [idx] = v;
    _b |= 1 << idx;

    int j = idx - 1;

    while (j >= 0 && ! (_b & (1 << j)))
        --j;

    if (j < 0) {
        while (++j != idx)
            _v [j] = v;
    } else {
        const float d = (_v [j] - v) / (j - idx);

        while (++j != idx)
            _v [j] = v + (j - idx) * d;
    }

    j = idx + 1;

    while ((j < N_NOTES) && ! (_b & (1 << j)))
        ++j;

    if (j > N_NOTES - 1) {
        while (--j != idx)
            _v [j] = v;
    } else {
        const float d = (_v [j] - v) / (j - idx);

        while (--j != idx)
            _v [j] = v + (j - idx) * d;
    }
}

void N_func::clearValue(const int idx)
{
    if (isPositiveAndBelow(idx, N_NOTES))
        return;

    const int m = 1 << idx;

    if (! (_b & m) || (_b == m))
        return;

    _b ^= m;

    int j = idx - 1;

    while ((j >= 0) && ! (_b & (1 << j)))
        --j;

    int k = idx + 1;

    while ((k <= N_NOTES - 1) && ! (_b & (1 << k)))
        ++k;

    if ((j >= 0) && (k < N_NOTES)) {
        const float d = (_v [k] - _v [j]) / (k - j);

        for (int i = j + 1; i < k; i++)
            _v [i] = _v [j] + (i - j) * d;
    } else if (j >= 0) {
        const float d = _v [j];

        while (j < N_NOTES - 1)
            _v [++j] = d;
    } else if (k < N_NOTES) {
        const float d = _v [k];

        while (k > 0)
            _v [--k] = d;
    }
}

float N_func::getValue(const int idx) const
{
    isPositiveAndBelow(idx, _v.size());
    return _v[idx];
}

bool N_func::isSet(const int idx) const
{
    isPositiveAndBelow(idx, _v.size());
    return (_b & (1 << idx)) != 0;
}

float N_func::operator[](const int note) const
{
    const int i = note / NOTES_GAP;
    const int k = note - NOTES_GAP * i;
    float v = _v [i];

    if (k) {
        // Apply linear interpolation if falls into the gap.
        isPositiveAndBelow(i + 1, _v.size());

        v += k * (_v [i + 1] - v) / NOTES_GAP;
    }

    return v;
}

void N_func::fromJson(const nlohmann::json& v)
{
    _b = v["mask"];

    if (auto varr = v["values"]; varr.is_array()) {
        if (varr.size() >= _v.size()) {
            for (int i = 0; i < _v.size(); ++i)
                _v[i] = varr[i];
        }
    }
}

void N_func::read(std::istream& stream)
{
    stream.read(reinterpret_cast<char*>(&_b), sizeof(int));

    for (int i = 0; i < _v.size(); ++i)
        stream.read(reinterpret_cast<char*>(&_v[i]), sizeof(float));
}
