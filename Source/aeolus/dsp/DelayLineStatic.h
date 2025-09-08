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

#include "aeolus/globals.h"

namespace dsp {
    /**
    * @brief Delay line with samples linear interpolation.
    */
template <size_t BUFFER_SIZE = 1024>
class DelayLineStatic {
    public:
        explicit DelayLineStatic() = default;

        static constexpr size_t size() noexcept { return BUFFER_SIZE; }

        void write (const float x) noexcept {
            if (writeIndex == 0) {
                writeIndex = buffer.size() - 1;
            } else {
                --writeIndex;
            }
            buffer[writeIndex] = x;
        }

        [[nodiscard]] float read(const float delay) const {
            auto index = static_cast<int>(std::floor(delay));
            auto frac = delay - static_cast<float>(index);
            index = (index + writeIndex) % static_cast<int>(buffer.size());
            const auto a = buffer[index];
            const auto b = index < buffer.size() - 1 ? buffer[index + 1] : buffer[0];

            return math::lerp(a, b, frac);
        }

        [[nodiscard]] float readNearest(const size_t delay) const noexcept {
            const auto index{ (delay + writeIndex) % buffer.size() };
            return buffer[index];
        }

    private:
        std::array<float, BUFFER_SIZE> buffer{ 0.0f };
        size_t writeIndex{ 0 };
    };
} // namespace dsp
