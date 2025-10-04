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
#include <ranges>
#include <vector>
#include <aeolus/Worker.h>
#define COMPLEX

template <size_t L>
class UniformPartitionedConvolver final {
public:
    static_assert(math::isPowerOfTwo(L), "Block length must be a power of two");

    UniformPartitionedConvolver() = default;
    ~UniformPartitionedConvolver() = default;

    void resizeAndReset(size_t n) {
        // Resize
#ifdef COMPLEX
        inputSpectrumBuffer.resize(L * 2 * n);
        irSpectrumBuffer.resize(L * 2 * n);
#else
        inputSpectrumBuffer.resize(L * 4 * n);
        irSpectrumBuffer.resize(L * 4 * n);
#endif


        printf("BEFORE: blocks.size() = %zu, n = %zu, blocks addr = %p\n",
               blocks.size(), n, (void*)&blocks);
        fflush(stdout);

        blocks.resize(n);

        printf("AFTER: blocks.size() = %zu\n", blocks.size());
        fflush(stdout);
        // Reset
        inputIndex = 0;
        inputSpectrumIndex = 0;
        irInputIndex = 0;
        irInputBlockIndex = 0;
#ifdef COMPLEX
        std::ranges::fill(irSpectrumBuffer, std::complex(0.0f, 0.0f));
        std::ranges::fill(inputSpectrumBuffer, std::complex(0.0f, 0.0f));
#else
        std::ranges::fill(irSpectrumBuffer, 0.0f);
        std::ranges::fill(inputSpectrumBuffer, 0.0f);
#endif
        size_t preconvolveIndex = 0;
        const size_t preconvolveIndexStep = blocks.empty() ? 0 : L / blocks.size();
        for (auto i = 0; i < blocks.size(); ++i) {
#ifdef COMPLEX
            blocks[i].inputSpectrumIterator = inputSpectrumBuffer.begin() + (i * L * 2);
            blocks[i].irSpectrumIterator = irSpectrumBuffer.begin() + (i * L * 2);
#else
            blocks[i].inputSpectrumIterator = inputSpectrumBuffer.begin() + (i * L * 4);
            blocks[i].irSpectrumIterator = irSpectrumBuffer.begin() + (i * L * 4);
#endif
            blocks[i].preconvolveIndex = preconvolveIndex;
            preconvolveIndex += preconvolveIndexStep;
            blocks[i].reset();
        }
    }

    void feedIr(const float x) {
        assert(irInputIndex < irSpectrumBuffer.size());
        assert(irInputBlockIndex < blocks.size());
#ifdef COMPLEX
        irSpectrumBuffer[irInputIndex] = std::complex(x, 0.0f);
        ++irInputIndex;
        if (irInputIndex % L == 0) {
            // IR input chunk is ready - compute ir Chunk spectrum
            blocks[irInputBlockIndex].irFFT();
            ++irInputBlockIndex;
            irInputIndex += L;
        }
#else
        irSpectrumBuffer[irInputIndex] = x;
        irInputIndex += 2;

        if (irInputIndex % (L * 2) == 0) {
            // IR input chunk is ready - compute ir Chunk spectrum
            blocks[irInputBlockIndex].irFFT();
            ++irInputBlockIndex;
            irInputIndex += L * 2;
        }
#endif
    }

    float tick(const float &x) {
        auto y = 0.0f;

        for (auto& block : blocks) {
            y += block.tick();
        }
#ifdef COMPLEX
        inputSpectrumBuffer[inputSpectrumIndex + inputIndex] = std::complex(x, 0.0f);
        ++inputIndex;
#else
        inputSpectrumBuffer[inputSpectrumIndex + 2 * inputIndex] = x;
        inputSpectrumBuffer[inputSpectrumIndex + 2 * inputIndex + 1] = 0.0f;
        ++inputIndex;
#endif
        if (inputIndex >= L) {
            // Input is ready - compute input spectrum
            inputIndex = 0;
            inputFft();
        }
        return y;
    }

    void inputFft() {
        // Clear padding
#ifdef COMPLEX
        GFFT<L * 2>::fft_real_padded(reinterpret_cast<float*>(&inputSpectrumBuffer[inputSpectrumIndex]));
#else
        ::memset(&inputSpectrumBuffer[inputSpectrumIndex + (L * 2)], 0, sizeof (float) * (L * 2));
        GFFT<L * 2>::fft_real_padded(&inputSpectrumBuffer[inputSpectrumIndex]);
#endif
        // Perform blocks convolution
        for (auto& block : blocks) {
            block.convolve();
        }

        // Rotate blocks input spectra
        auto i = blocks.size() - 1;
        auto lastBlockInputSpectrumIterator = blocks[i].inputSpectrumIterator;
        for (; i > 0; --i) {
            blocks[i].inputSpectrumIterator = blocks[i - 1].inputSpectrumIterator;
        }
        blocks[0].inputSpectrumIterator = lastBlockInputSpectrumIterator;

        // Move the input spectrum index to the next chunk
#ifdef COMPLEX
        inputSpectrumIndex = inputSpectrumIndex == 0 ? inputSpectrumBuffer.size() - (L * 2) : inputSpectrumIndex - (L * 2);
#else
        inputSpectrumIndex = inputSpectrumIndex == 0 ? inputSpectrumBuffer.size() - (L * 4) : inputSpectrumIndex - (L * 4);
#endif

        // First block receives fresh input signal and cannot be dephased.
        blocks[0].dephase = false;
    }

private:
    class Block final {
    public:
#ifdef COMPLEX
        std::vector<std::complex<float>>::iterator inputSpectrumIterator;
        std::vector<std::complex<float>>::iterator irSpectrumIterator;
#else
        std::vector<float>::iterator inputSpectrumIterator;
        std::vector<float>::iterator irSpectrumIterator;
#endif

        alignas(32) std::array<std::complex<float>, L * 2> convolutionBuffer{ std::complex(0.0f, 0.0f) };
        alignas(32) std::array<float, L> outputBuffer{ 0.0f };
        alignas(32) std::array<float, L> tailBuffer{ 0.0f };

        size_t tailIndex{ 0 };
        bool irReady{ false };
        bool dephase{ false };
        size_t preconvolveIndex{ 0 };
        std::atomic<bool> preconvolved{ false };

        Block() = default;
        Block(const Block& other) : inputSpectrumIterator{other.inputSpectrumIterator}, irSpectrumIterator{other.irSpectrumIterator} {}
        ~Block() = default;

        void reset() {
            convolutionBuffer.fill(std::complex(0.0f, 0.0f));
            outputBuffer.fill(0.0f);
            tailBuffer.fill(0.0f);
            tailIndex = 0;
            irReady = false;
            dephase = false;
            preconvolved = false;
        }

        void irFFT() {
            GFFT<L * 2>::fft_real_padded(reinterpret_cast<float*>(&(*irSpectrumIterator)));
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

        float tick() {
            float y = outputBuffer[tailIndex];
            tailIndex = (tailIndex + 1) % L;

            if (dephase && tailIndex == preconvolveIndex) {
                preconvolve();
            }

            return y;
        }

    private:
        void preconvolve() {
            if (preconvolved){
                return; // Already preconvolved or overload?
            }
            SIMD::complex_mul_conj(reinterpret_cast<float*>(convolutionBuffer.data()), reinterpret_cast<float*>(&(*inputSpectrumIterator)), reinterpret_cast<float*>(&(*irSpectrumIterator)), L * 4);
            GFFT<L * 2>::fft(reinterpret_cast<float*>(convolutionBuffer.data()));
            preconvolved = true;
        }

        void postconvolve() {

            if (!preconvolved) {
                return; // Not ready
            }
            // Add tail buffer from the previous convolution
            for (size_t i = 0; i < L; ++i) {
                constexpr float norm = 1.0f / (L * 2);
                // Put output samples together without interleaving
                outputBuffer[i] = norm * (convolutionBuffer[i].real() + tailBuffer[i]);
                tailBuffer[i] = convolutionBuffer[L + i].real();
            }
            preconvolved = false;
        }
    };

    size_t inputIndex{ 0 };
    size_t inputSpectrumIndex{ 0 };
    size_t irInputIndex{ 0 };
#ifdef COMPLEX
    alignas(32) std::vector<std::complex<float>> inputSpectrumBuffer{ std::complex(0.0f, 0.0f) };
    alignas(32) std::vector<std::complex<float>> irSpectrumBuffer{ std::complex(0.0f, 0.0f) };
#else
    alignas(32) std::vector<float> inputSpectrumBuffer{ 0.0f };
    alignas(32) std::vector<float> irSpectrumBuffer{ 0.0f };
#endif
    std::vector<Block> blocks;
    size_t irInputBlockIndex{ 0 };
};
