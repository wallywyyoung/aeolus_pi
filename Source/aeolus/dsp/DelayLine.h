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
#include <cstdint>
#include <cstring>
#include <vector>

#include "aeolus/globals.h"

namespace dsp {
    /**
    * @brief Delay line with samples linear interpolation.
    */
    class DelayLine {
    public:
        explicit DelayLine(const size_t size = 1024) : buffer(size, 0.0f) { }

        void resize(const size_t size) {
            buffer.resize(size);
            reset();
        }

        void reset() {
            writeIndex = 0;
            std::memset(buffer.data(), 0, sizeof (float) * buffer.size());
        }

        void write(const float x) noexcept {
            writeIndex = writeIndex == 0 ? buffer.size() - 1 : --writeIndex;
            buffer[writeIndex] = x;
        }

        [[nodiscard]] float read(const float &delay) const {
            assert(delay >= 0.0f);
            const auto integral = std::floor(delay);
            const auto fraction = delay - integral;

            auto index = (static_cast<size_t>(integral) + writeIndex) % buffer.size();
            assert(index < buffer.size());
            const auto a = buffer[index];
            const auto b = index < buffer.size() - 1 ? buffer[index + 1] : buffer[0];

            return math::lerp(a, b, fraction);
        }

        [[nodiscard]] float readNearest(const int &delay) const {
            assert(delay >= 0 && (delay + writeIndex) % buffer.size() < buffer.size());
            return buffer[(delay + writeIndex) % buffer.size()];
        }

        [[nodiscard]] size_t size() const { return buffer.size(); }
    private:
        std::vector<float> buffer;
        size_t writeIndex{ 0 };
    };
} // namespace dsp
