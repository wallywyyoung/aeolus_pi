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

#pragma once

#include <array>
#include <cmath>
#include <vector>

#include "aeolus/globals.h"

namespace dsp {
    /**
    * @brief Delay line with samples linear interpolation.
    */
template <size_t BUFFER_SIZE = 1024>
class DelayLineStatic {
    public:
        explicit DelayLineStatic() = default;

        constexpr size_t size() noexcept { return BUFFER_SIZE; }

        void write (const float x) noexcept {
            if (_writeIndex == 0)
                _writeIndex = _buffer.size() - 1;
            else
                --_writeIndex;

            _buffer[_writeIndex] = x;
        }

        [[nodiscard]] float read(const float delay) const {
            int index = (int)std::floor(delay);
            float frac = delay - (float)index;

            index = (index + _writeIndex) % (int) _buffer.size();
            const auto a = _buffer[index];
            const auto b = index < _buffer.size() - 1 ? _buffer[index + 1] : _buffer[0];

            return math::lerp(a, b, frac);
        }

        [[nodiscard]] float readNearest(const int delay) const noexcept{
            const int index{ int ((delay + _writeIndex) % _buffer.size()) };
            return _buffer[index];
        }

    private:
        std::array<float, BUFFER_SIZE> _buffer{ 0.0f };
        size_t _writeIndex{ 0 };
    };
} // namespace dsp
