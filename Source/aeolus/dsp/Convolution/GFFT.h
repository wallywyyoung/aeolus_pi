#pragma once

#include <array>
#include <complex>
#include "aeolus/dsp/Convolution/DanielsonLanczos.h"

template<unsigned N, typename T = float>
struct GFFT {
    using Impl = DanielsonLanczos<N, T>;

    // Data format: [RIRIRIRI...]
    static void fft(T* data)
    {
        permutate (data);
        Impl::apply (data);
    }

    // Data format: [R0R0R0R0...]
    static void fft_real(T* data) {
        permutate_real (data);
        Impl::apply_real (data);
    }

    // Data format: [R0R0R0...000000]
    static void fft_real_padded(T* data) {
        permutate_real_padded (data);
        Impl::apply_real_padded (data);
    }

    static void ifft(T* data) {
        conj (data);
        fft (data);
        conj (data);

        constexpr float norm = 1.0f / N;

        for (unsigned i = 0; i < 2*N; ++i)
            data[i] *= norm;
    }

    template <typename U>
    static constexpr U log2int(U n) {
        return ((n < 2) ? 0 : 1 + log2int(n / 2));
    }

    static void permutate(T* data) {
        constexpr unsigned m = log2int (N);

        for (unsigned a = 0; a < N; a++) {
            unsigned b = a;

            // Reverse bits
            b = (((b & 0xaaaaaaaa) >> 1) | ((b & 0x55555555) << 1));
            b = (((b & 0xcccccccc) >> 2) | ((b & 0x33333333) << 2));
            b = (((b & 0xf0f0f0f0) >> 4) | ((b & 0x0f0f0f0f) << 4));
            b = (((b & 0xff00ff00) >> 8) | ((b & 0x00ff00ff) << 8));
            b = ((b >> 16) | (b << 16)) >> (32 - m);

            if (b > a) {
                std::swap(data[a * 2], data[b * 2]);
                std::swap(data[a * 2 + 1], data[b * 2 + 1]);
            }
        }
    }

    static void permutate_real(T* data) {
        constexpr unsigned m = log2int(N);

        for (unsigned a = 0; a < N; a++) {
            unsigned b = a;

            // Reverse bits
            b = (((b & 0xaaaaaaaa) >> 1) | ((b & 0x55555555) << 1));
            b = (((b & 0xcccccccc) >> 2) | ((b & 0x33333333) << 2));
            b = (((b & 0xf0f0f0f0) >> 4) | ((b & 0x0f0f0f0f) << 4));
            b = (((b & 0xff00ff00) >> 8) | ((b & 0x00ff00ff) << 8));
            b = ((b >> 16) | (b << 16)) >> (32 - m);

            if (b > a)
                std::swap(data[a * 2], data[b * 2]);
        }
    }

    static void permutate_real_padded(T* data) {
        constexpr unsigned m = log2int(N);

        for (unsigned a = 0; a < N / 2; a++) {
            unsigned b = a;

            // Reverse bits
            b = (((b & 0xaaaaaaaa) >> 1) | ((b & 0x55555555) << 1));
            b = (((b & 0xcccccccc) >> 2) | ((b & 0x33333333) << 2));
            b = (((b & 0xf0f0f0f0) >> 4) | ((b & 0x0f0f0f0f) << 4));
            b = (((b & 0xff00ff00) >> 8) | ((b & 0x00ff00ff) << 8));
            b = ((b >> 16) | (b << 16)) >> (32 - m);

            if (b > a)
                std::swap(data[a * 2], data[b * 2]);
        }
    }

    static void conj(T* data) {
        for (unsigned i = 1; i < 2 * N; i += 2) {
            data[i] = -data[i];
        }
    }
};
