#pragma once

#include <array>
#include "GFFT.h"

template<size_t L>
struct DataFFT {
    alignas(32) std::array<std::complex<float>, L * 2> complexInputBuffer{ 0.0f };
    alignas(32) std::array<std::complex<float>, L * 2> complexIrBuffer{ 0.0f };
    alignas(32) std::array<float, L> tailBuffer{ 0.0f };
    size_t tailIndex{ 0 };
    size_t readIndex{ 0 };

    void reset(const size_t& inputSize) {
        complexInputBuffer.fill({0.0f, 0.0f});
        complexIrBuffer.fill({0.0f, 0.0f});
        tailBuffer.fill(0.0f);
        tailIndex = 0;
        readIndex = (inputSize - 1) % inputSize;
    }
};