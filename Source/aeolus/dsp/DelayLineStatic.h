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

#include <algorithm>
#include <array>
#include <cmath>
#include "aeolus/utilities/SimdUtilities.h"

template <size_t BUFFER_SIZE = 1024, size_t PROCESS_SIZE = BUFFER_SIZE>
class DelayLineStatic {
    static_assert(BUFFER_SIZE >= PROCESS_SIZE * 4, "Delay line needs to be several times larger than process frames or the circularBuffer will overwrite itself.");

    std::array<float, BUFFER_SIZE> circularBuffer{ 0.0f };
    size_t writeIndex{ 0 };

    static void copyReverseCircular(const float* buffer, float* dst, const size_t nElements, const size_t bufferSize, const size_t index) noexcept {
        const auto firstHalfNumberElements = bufferSize - index;
        const auto secondHalfNumberElements = nElements - firstHalfNumberElements;
        std::reverse_copy(buffer, buffer + secondHalfNumberElements, dst);
        std::reverse_copy(buffer + index, buffer + index + firstHalfNumberElements, dst + secondHalfNumberElements);
    }

public:
    explicit DelayLineStatic() = default;

    static constexpr size_t size() noexcept { return BUFFER_SIZE; }

    void reset() {
        writeIndex = 0;
        circularBuffer.fill(0.0f);
    }

    // TODO: Fix tremulant so I can remove this.
    void write (const float x) noexcept {
        if (writeIndex == 0) {
            writeIndex = circularBuffer.size() - 1;
        } else {
            --writeIndex;
        }
        circularBuffer[writeIndex] = x;
    }

    // TODO: Fix tremulant so I can remove this.
    [[nodiscard]] float read(const float delay) const {
        auto index = static_cast<int>(std::floor(delay));
        auto frac = delay - static_cast<float>(index);
        index = (index + writeIndex) % static_cast<int>(circularBuffer.size());
        const auto a = circularBuffer[index];
        const auto b = index < circularBuffer.size() - 1 ? circularBuffer[index + 1] : circularBuffer[0];
        return std::lerp(a, b, frac);
    }

    void process(std::array<float, PROCESS_SIZE> &src, const size_t delay) noexcept {
        writeIndex = (writeIndex + circularBuffer.size() - src.size()) % circularBuffer.size();
        const auto delayIndex = (delay + writeIndex) % circularBuffer.size();
        if (circularBuffer.size() - writeIndex >= src.size()) {
            std::reverse_copy(src.begin(), src.end(), &circularBuffer[writeIndex]);
        } else {
            const auto firstHalfNumberElements = circularBuffer.size() - writeIndex;
            std::reverse_copy(src.end() - firstHalfNumberElements, src.end(), circularBuffer.begin() + writeIndex);
            std::reverse_copy(src.begin(), src.end() - firstHalfNumberElements, circularBuffer.begin());
        }
        if (delayIndex + src.size() <= circularBuffer.size()) {
            std::reverse_copy(circularBuffer.begin() + delayIndex, circularBuffer.begin() + delayIndex + src.size(), src.begin());
        } else {
            copyReverseCircular(circularBuffer.begin(), src.begin(), src.size(), circularBuffer.size(), delayIndex);
        }
    }

    void writeBuffer(std::array<float, PROCESS_SIZE> &src) {
        writeIndex = (writeIndex + circularBuffer.size() - src.size()) % circularBuffer.size();
        if (circularBuffer.size() - writeIndex >= src.size()) {
            std::reverse_copy(src.begin(), src.end(), &circularBuffer[writeIndex]);
        } else {
            const auto firstHalfNumberElements = circularBuffer.size() - writeIndex;
            std::reverse_copy(src.end() - firstHalfNumberElements, src.end(), circularBuffer.begin() + writeIndex);
            std::reverse_copy(src.begin(), src.end() - firstHalfNumberElements, circularBuffer.begin());
        }
    }

    void processLerpMono(std::array<float, PROCESS_SIZE> &src, const float delay) {
        float integral;
        const float fraction = std::modf(delay, &integral);
        if (fraction == 0.0f) {
            process(src, delay);
            return;
        }
        writeIndex = (writeIndex + circularBuffer.size() - src.size()) % circularBuffer.size();
        const auto delayIndex = (static_cast<size_t>(integral) + writeIndex) % circularBuffer.size();
        if (circularBuffer.size() - writeIndex >= src.size()) {
            std::reverse_copy(src.begin(), src.end(), &circularBuffer[writeIndex]);
        } else {
            const auto firstHalfNumberElements = circularBuffer.size() - writeIndex;
            std::reverse_copy(src.end() - firstHalfNumberElements, src.end(), circularBuffer.begin() + writeIndex);
            std::reverse_copy(src.begin(), src.end() - firstHalfNumberElements, circularBuffer.begin());
        }

        if (delayIndex + src.size() <= circularBuffer.size()) {
            std::reverse_copy(circularBuffer.begin() + delayIndex, circularBuffer.begin() + delayIndex + src.size(), src.begin());
        } else {
            copyReverseCircular(circularBuffer.begin(), src.begin(), src.size(), circularBuffer.size(), delayIndex);
            const auto firstHalfNumberElements = circularBuffer.size() - delayIndex;
            const auto secondHalfNumberElements = src.size() - firstHalfNumberElements;
            std::reverse_copy(circularBuffer.begin(), circularBuffer.begin() + secondHalfNumberElements, src.begin());
            std::reverse_copy(circularBuffer.begin() + delayIndex, circularBuffer.begin() + delayIndex + firstHalfNumberElements, src.begin() + secondHalfNumberElements);
        }

        if (delayIndex + src.size() < circularBuffer.size() - 1) {
            SimdUtilities::reverseLerpF32(&circularBuffer[delayIndex], &circularBuffer[delayIndex + 1], fraction, src.begin(), src.size());
        } else {
            const auto firstHalfSize = circularBuffer.size() - delayIndex - 1;
            SimdUtilities::reverseLerpF32(&circularBuffer[delayIndex], &circularBuffer[delayIndex + 1], fraction, src.begin(), firstHalfSize);
            SimdUtilities::reverseLerpF32(&circularBuffer[0], &circularBuffer[1], fraction, src.begin() + firstHalfSize, src.size() - firstHalfSize);
        }
    }
};