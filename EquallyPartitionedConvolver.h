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

#include <atomic>
#include <cassert>
#include <vector>
#include "aeolus/SIMD.h"
#include "aeolus/dsp/Convolution/GFFT.h"
#include "aeolus/globals.h"

template <size_t L>
class EquallyPartitionedConvolver {
    static_assert(math::isPowerOfTwo(L), "Block length must be a power of two");
    constexpr static size_t SPECTRUM_UNPADDED_SIZE = 2 * L;
    constexpr static size_t SPECTRUM_PADDED_SIZE = 4 * L;

    struct Block final {
        float* inputSpectrumPtr{ nullptr };
        float* irSpectrumPtr{ nullptr };

        alignas(32) std::array<float, SPECTRUM_PADDED_SIZE> convolutionBuffer;
        alignas(32) std::array<float,L> outputBuffer;
        alignas(32) std::array<float,L> tailBuffer;

        size_t tailIndex{ 0 };
        size_t preconvolveIndex{ 0 };

        bool irReady{ false };
        bool dephase{ false };

        std::atomic<bool> preconvolved{ false };

        Block() = default;

        Block(const Block& other) : inputSpectrumPtr{other.inputSpectrumPtr}, irSpectrumPtr{other.irSpectrumPtr} { }

        ~Block() = default;

        void reset() {
            convolutionBuffer.fill(0.0f);
            outputBuffer.fill(0.0f);
            tailBuffer.fill(0.0f);

            tailIndex = 0;
            irReady = false;
            dephase = false;
            preconvolved =false;
        }

        void irFft() {
            assert(irSpectrumPtr != nullptr);
            GFFT<SPECTRUM_UNPADDED_SIZE>::fft_real_padded(irSpectrumPtr);
            irReady = true;
        }

        void convolve() {
            // IR is not ready yet - nothing to do
            if (!irReady) {
                return;
            }

            if (dephase) {
                postconvolve();
                return;
            }

            preconvolve();
            postconvolve();

            dephase = true;
        }

        void preconvolve() {
            assert(inputSpectrumPtr != nullptr);
            assert(irSpectrumPtr != nullptr);

            if (preconvolved) {
                return;
            }

            SIMD::complex_mul_conj(convolutionBuffer.data(), inputSpectrumPtr, irSpectrumPtr, SPECTRUM_PADDED_SIZE);
            GFFT<SPECTRUM_UNPADDED_SIZE>::fft(convolutionBuffer.data());
            preconvolved = true;
        }

        void postconvolve() {
            if (!preconvolved) {
                return;
            }

            // Add the tail buffer from the previous convolution
            size_t tail = 0;
            for (size_t i = 0; i < SPECTRUM_UNPADDED_SIZE; i += 2) {
                constexpr float norm = 1.0f / SPECTRUM_UNPADDED_SIZE;
                // Put output samples together without interleaving
                outputBuffer[tail] = norm * (convolutionBuffer[i] + tailBuffer[tail]);
                tailBuffer[tail] = convolutionBuffer[SPECTRUM_UNPADDED_SIZE + i];
                ++tail;
            }

            preconvolved = false;
        }

        float tick() {
            const auto y = outputBuffer[tailIndex];
            tailIndex = (tailIndex + 1) % L;

            if (dephase && tailIndex == preconvolveIndex) {
                preconvolve();
            }

            return y;
        }
    };

    size_t inputIndex{ 0 };

    alignas(32) std::vector<float> inputSpectrumBuffer{};
    size_t inputSpectrumIndex{ 0 };

    alignas(32) std::vector<float> irSpectrumBuffer{};
    size_t irInputIndex{ 0 };
    size_t irInputBlockIndex{ 0 };

    std::vector<Block> blocks{};

public:
    EquallyPartitionedConvolver() = default;

    ~EquallyPartitionedConvolver() = default;

    void resize(size_t n) {
        irSpectrumBuffer.resize(n * SPECTRUM_PADDED_SIZE);
        inputSpectrumBuffer.resize(n * SPECTRUM_PADDED_SIZE);
        blocks.resize(n);
        reset();
    }

    void reset() {
        inputIndex = 0;
        inputSpectrumIndex = 0;
        irInputIndex = 0;
        irInputBlockIndex = 0;

        std::fill(inputSpectrumBuffer.begin(), inputSpectrumBuffer.end(), 0.0f);
        std::fill(irSpectrumBuffer.begin(), irSpectrumBuffer.end(), 0.0f);

        size_t preconvolveIndex = 0;
        const size_t preconvolveIndexStep = blocks.empty() ? 0 : L / blocks.size();
        for (size_t i = 0; i < blocks.size(); ++i) {
            blocks[i].inputSpectrumPtr = &inputSpectrumBuffer[i * SPECTRUM_PADDED_SIZE];
            blocks[i].irSpectrumPtr = &irSpectrumBuffer[i * SPECTRUM_PADDED_SIZE];
            blocks[i].preconvolveIndex = preconvolveIndex;
            preconvolveIndex += preconvolveIndexStep;
            blocks[i].reset();
        }
    }

    void feedIr(const float& x) {
        assert(irInputIndex < irSpectrumBuffer.size());
        assert(irInputBlockIndex < blocks.size());

        irSpectrumBuffer[irInputIndex] = x;
        irInputIndex += 2;

        if (irInputIndex % SPECTRUM_UNPADDED_SIZE == 0) {
            // IR input chunk is ready - compute IR chunk spectrum
            blocks[irInputBlockIndex].irFft();
            ++irInputBlockIndex;
            irInputIndex += SPECTRUM_UNPADDED_SIZE;
        }
    }

    float tick(const float& x) {
        auto y = 0.0f;

        for (auto& block : blocks) {
            y += block.tick();
        }

        inputSpectrumBuffer[inputSpectrumIndex + 2 * inputIndex] = x;
        inputSpectrumBuffer[inputSpectrumIndex + 2 * inputIndex + 1] = 0.0f;
        ++inputIndex;

        if (inputIndex >= L) {
            // Input is ready - compute input spectrum
            inputIndex = 0;
            inputFft();
        }

        return y;
    }

    void inputFft() {
        // Clear padding
        std::fill(inputSpectrumBuffer.begin() + inputSpectrumIndex + SPECTRUM_UNPADDED_SIZE, inputSpectrumBuffer.end(), 0.0f);
        GFFT<SPECTRUM_UNPADDED_SIZE>::fft_real_padded(&inputSpectrumBuffer[inputSpectrumIndex]);

        // Perform blocks convolution
        for (auto& block : blocks) {
            block.convolve();
        }

        // Rotate blocks input spectra
        float* lastBlockInputSpectrumPtr = blocks[blocks.size() - 1].inputSpectrumPtr;
        for (size_t i = blocks.size() - 1; i > 0; --i) {
            blocks[i].inputSpectrumPtr = blocks[i - 1].inputSpectrumPtr;
        }
        blocks[0].inputSpectrumPtr = lastBlockInputSpectrumPtr;

        // Move the input spectrum index to the next chunk
        inputSpectrumIndex = inputSpectrumIndex == 0 ? inputSpectrumBuffer.size() - SPECTRUM_PADDED_SIZE : inputSpectrumIndex - SPECTRUM_PADDED_SIZE;

        // First block receives fresh input signal and cannot be dephased.
        blocks[0].dephase = false;
    }
};