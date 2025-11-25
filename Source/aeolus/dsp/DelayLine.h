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
/**
 * @brief Delay line with samples linear interpolation.
 */
class DelayLine {
public:
    explicit DelayLine(const size_t size = 1024) : buffer{static_cast<float *>(malloc(size * sizeof(float)))}, bufferSize(size) {
        arm_fill_f32(0.0f, buffer, bufferSize);
    }

    ~DelayLine() { free(buffer); }

    void resize(const size_t size) {
        free(buffer);
        bufferSize = size;
        buffer = static_cast<float *>(malloc(size * sizeof(float)));
        reset();
    }

    void reset() {
        writeIndex = 0;
        arm_fill_f32(0.0f, buffer, bufferSize);
    }

    static void copyReverseCircular(const float* buffer, float* dst, const size_t nElements, const size_t bufferSize, const size_t index) noexcept {
        const auto firstHalfNumberElements = bufferSize - index;
        const auto secondHalfNumberElements = nElements - firstHalfNumberElements;
        std::reverse_copy(buffer, buffer + secondHalfNumberElements, dst);
        std::reverse_copy(buffer + index, buffer + index + firstHalfNumberElements, dst + secondHalfNumberElements);
    }

    void process(const std::array<float, PROCESS_FRAMES_SIZE> &in, float *outL, float *outR, const size_t delayL, const size_t delayR) noexcept {
        writeIndex = (writeIndex + bufferSize - in.size()) % bufferSize;
        const auto delayIndexL = (delayL + writeIndex) % bufferSize;
        const auto delayIndexR = (delayR + writeIndex) % bufferSize;
        if (bufferSize - writeIndex >= in.size()) {
            std::reverse_copy(in.begin(), in.end(), &buffer[writeIndex]);
        } else {
            const auto firstHalfNumberElements = bufferSize - writeIndex;
            std::reverse_copy(in.end() - firstHalfNumberElements, in.end(), buffer + writeIndex);
            std::reverse_copy(in.begin(), in.end() - firstHalfNumberElements, buffer);
        }
        if (delayIndexL + in.size() <= bufferSize) {
            std::reverse_copy(buffer + delayIndexL, buffer + delayIndexL + in.size(), outL);
        } else {
            copyReverseCircular(buffer, outL, in.size(), bufferSize, delayIndexL);
        }
        if (delayIndexR + in.size() <= bufferSize) {
            std::reverse_copy(buffer + delayIndexR, buffer + delayIndexR + in.size(), outR);
        } else {
            copyReverseCircular(buffer, outR, in.size(), bufferSize, delayIndexR);
        }
    }

    [[nodiscard]] size_t size() const {
        return bufferSize;
    }

private:
    float* buffer;
    size_t bufferSize{ 0 };
    size_t writeIndex{ 0 };
};
