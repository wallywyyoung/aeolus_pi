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

#include "aeolus/SIMD.h"
#include "aeolus/dsp/Convolution/GFFT.h"
#include "aeolus/dsp/Convolution/DataFFT.h"
#include "aeolus/globals.h"

class FFT {
public:
    template <size_t L>
    static void init(const float* irBuffer, const size_t& inputSize, DataFFT<L>& data) {
        data.readIndex = (inputSize - 1) % inputSize;
        std::copy(irBuffer + L, irBuffer + L * 2, data.complexIrBuffer.begin());
        std::fill(data.complexIrBuffer.begin() + L, data.complexIrBuffer.end(), std::complex(0.0f, 0.0f));
        // std::complex will work into a two-element pointer
        GFFT<L * 2>::fft_real_padded(reinterpret_cast<float*>(data.complexIrBuffer.data()));
    }

    template <size_t L>
    static float tick(const float* inputBuffer, const size_t& inputSize, DataFFT<L>& data) {
        static_assert(math::isPowerOfTwo(L), "Convolution part length must be a power of two");
        const float y = data.complexInputBuffer[data.tailIndex].real();

        // Feed from the input buffer
        const size_t idx = (data.readIndex - data.tailIndex) % inputSize;
        data.complexInputBuffer[data.tailIndex] = {inputBuffer[idx], 0.0f};
        data.tailIndex = (data.tailIndex + 1) % L;

        if (data.tailIndex != 0) {
            return y;
        }

        std::fill(data.complexInputBuffer.data() + (data.complexInputBuffer.size() / 2), data.complexInputBuffer.end(), std::complex(0.0f, 0.0f));
        GFFT<L * 2>::fft_real_padded(reinterpret_cast<float*>(data.complexInputBuffer.data()));
        SIMD::complex_mul_conj(
            reinterpret_cast<float*>(data.complexInputBuffer.data()),
            reinterpret_cast<float*>(data.complexInputBuffer.data()),
            reinterpret_cast<float*>(data.complexIrBuffer.data()),
            data.complexIrBuffer.size() * 2);
        GFFT<L * 2>::fft(reinterpret_cast<float*>(data.complexInputBuffer.data()));
        // Add tail buffer from the previous convolution
        size_t tail = 0;
        for (auto i = 0; i < L; ++i) {
            constexpr float NORM = 1.0f / static_cast<float>(L * 2);
            // Put output samples together without interleaving
            data.complexInputBuffer[i] = NORM * (data.complexInputBuffer[i].real() + data.tailBuffer[tail]);
            data.tailBuffer[tail] = data.complexInputBuffer[L + i].real();
            ++tail;
        }

        data.readIndex = (data.readIndex - L) % inputSize;
        return y;
    }
};
