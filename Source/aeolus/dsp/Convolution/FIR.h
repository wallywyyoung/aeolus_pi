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

#include <bit>
#include "aeolus/SIMD.h"

class FIR {
public:
    template<size_t L>
    static float tick(const float* irBuffer, const float* inputBuffer, const size_t& inputSize, size_t& readIndex) {
    static_assert(std::has_single_bit(L), "Convolution part length must be a power of two");
        auto y = 0.0f;
        if (readIndex + L < inputSize) {
            y = SIMD::mul_reduce_unaligned(irBuffer, &inputBuffer[readIndex], L);
        } else {
            for (size_t i = 0; i < L; ++i)
                y += irBuffer[i] * inputBuffer[(readIndex + i) % inputSize];
        }
        return y;
    }
};