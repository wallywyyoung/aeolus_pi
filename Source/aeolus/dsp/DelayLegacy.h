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

template <size_t BUFFER_SIZE = 1024, size_t PROCESS_SIZE = BUFFER_SIZE>
class DelayLegacy {
    static_assert(BUFFER_SIZE >= PROCESS_SIZE * 4, "Delay line needs to be several times larger than process frames or the circularBuffer will overwrite itself.");
    std::array<float, BUFFER_SIZE> circularBuffer{ 0.0f };
    size_t writeIndex{ 0 };
public:
    void write (const float x) noexcept {
        if (writeIndex == 0) {
            writeIndex = circularBuffer.size() - 1;
        } else {
            --writeIndex;
        }
        circularBuffer[writeIndex] = x;
    }
    [[nodiscard]] float read(const float delay) const {
        auto index = static_cast<int>(std::floor(delay));
        auto frac = delay - static_cast<float>(index);
        index = (index + writeIndex) % static_cast<int>(circularBuffer.size());
        const auto a = circularBuffer[index];
        const auto b = index < circularBuffer.size() - 1 ? circularBuffer[index + 1] : circularBuffer[0];
        return std::lerp(a, b, frac);
    }
};
