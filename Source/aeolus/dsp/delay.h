// ----------------------------------------------------------------------------
//
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

#pragma once

#include <cmath>
#include <vector>
#include <cstring>

#include "aeolus/globals.h"

namespace dsp {
    /**
    * @brief Delay line with samples linear interpolation.
    */
    class DelayLine {
    public:
        explicit DelayLine(size_t size = 1024);
        void resize(size_t size);
        void reset();
        void write(float x);
        [[nodiscard]] float read(float delay) const;
        [[nodiscard]] float readNearest(int delay) const;
        [[nodiscard]] size_t size() const { return _buffer.size(); }
    private:
        std::vector<float> _buffer;
        size_t _writeIndex;
    };

    template <size_t BUFFER_SIZE>
    class DelayLineStatic
    {
    public:
        DelayLineStatic() = default;

        void reset() {
            _writeIndex = 0;
            memset(_buffer, 0, sizeof(float) * BUFFER_SIZE);
        }

        void write (const float x) {
            if (_writeIndex == 0) {
                _writeIndex = BUFFER_SIZE - 1;
            } else {
                --_writeIndex;
            }

            _buffer[_writeIndex] = x;
        }

        float read(const float delay) const
        {
            int index = static_cast<int>(std::floor(delay));
            const float frac = delay - static_cast<float>(index);

            index = (index + _writeIndex) % static_cast<int>(BUFFER_SIZE);
            const auto a = _buffer[index];
            const auto b = index < BUFFER_SIZE - 1 ? _buffer[index + 1] : _buffer[0];

            return math::lerp(a, b, frac);
        }

        float readNearest(const int delay) const
        {
            const int index{ static_cast<int>((delay + _writeIndex) % BUFFER_SIZE) };
            return _buffer[index];
        }
    private:
        float _buffer[BUFFER_SIZE] = {};
        size_t _writeIndex{};
    };
} // namespace dsp
