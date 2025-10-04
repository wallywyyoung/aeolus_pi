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

#include "aeolus/dsp/Convolution/DataFFT.h"
#include "aeolus/dsp/Convolution/FFT.h"
#include "aeolus/dsp/Convolution/FIR.h"

class CascadeConvolver {
    DataFFT<32> data32{};
    DataFFT<64> data64{};
    DataFFT<128> data128{};
    DataFFT<256> data256{};
    DataFFT<512> data512{};
    DataFFT<1024> data1024{};
    DataFFT<2048> data2048{};

    float* irBuffer;
    float* inputBuffer;
    size_t firReadIndex{ 0 };
public:
    constexpr static size_t BLOCK_SIZE = 4096;

    CascadeConvolver() = default;

    void init(float* ir, float* input) {
        assert(input != nullptr);
        assert(ir != nullptr);
        irBuffer = ir;
        inputBuffer = input;
        firReadIndex = (BLOCK_SIZE - 1) % BLOCK_SIZE;
        FFT::init(irBuffer, BLOCK_SIZE, data32);
        FFT::init(irBuffer, BLOCK_SIZE, data64);
        FFT::init(irBuffer, BLOCK_SIZE, data128);
        FFT::init(irBuffer, BLOCK_SIZE, data256);
        FFT::init(irBuffer, BLOCK_SIZE, data512);
        FFT::init(irBuffer, BLOCK_SIZE, data1024);
        FFT::init(irBuffer, BLOCK_SIZE, data2048);
    }

    void reset() {
        firReadIndex = (BLOCK_SIZE - 1) % BLOCK_SIZE;
        data32.reset(BLOCK_SIZE);
        data64.reset(BLOCK_SIZE);
        data128.reset(BLOCK_SIZE);
        data256.reset(BLOCK_SIZE);
        data512.reset(BLOCK_SIZE);
        data1024.reset(BLOCK_SIZE);
        data2048.reset(BLOCK_SIZE);
    }

    float tick(const float &x) {
        firReadIndex = (firReadIndex - 1) % BLOCK_SIZE;
        inputBuffer[firReadIndex] = x;
        auto result = FIR::tick<32>(irBuffer, inputBuffer, BLOCK_SIZE, firReadIndex);
        result += FFT::tick(inputBuffer, BLOCK_SIZE, data32);
        result += FFT::tick(inputBuffer, BLOCK_SIZE, data64);
        result += FFT::tick(inputBuffer, BLOCK_SIZE, data128);
        result += FFT::tick(inputBuffer, BLOCK_SIZE, data256);
        result += FFT::tick(inputBuffer, BLOCK_SIZE, data512);
        result += FFT::tick(inputBuffer, BLOCK_SIZE, data1024);
        result += FFT::tick(inputBuffer, BLOCK_SIZE, data2048);
        return result;
    }
};
