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
#include <complex>
#include <valarray>
#include "aeolus/SIMD.h"
#include "aeolus/globals.h"

namespace dsp {

/**
 * @brief Fast Fourier transformation of a complex array.
 */
class Fft {
public:
    enum class Window{ None, Hann, Hamming, Blackman };

    static void direct(std::valarray<std::complex<float>>& x, Window win = Window::None);

    static void inverse(std::valarray<std::complex<float>>& x);

private:

    static void applyWindow(std::valarray<std::complex<float>>& x, Window win);
};

//----------------------------------------------------------

template<unsigned N, typename T = float>
struct DanielsonLanczos;

template<unsigned N, typename T>
struct DanielsonLanczos
{
    using Next = DanielsonLanczos<N / 2, T>;

    inline static std::array<float, N> w alignas(32) = [] {
        std::array<float, N> w;

        w[0] = 1.0f;
        w[1] = 0.0f;

        T wtemp, wr, wi, wpr, wpi;

        wtemp = math::Sin<N, 1, T>::value;
        wpr = -2.0f * wtemp * wtemp;
        wpi = -math::Sin<N, 2, T>::value;
        wr = 1.0f + wpr;
        wi = wpi;

        for (size_t i = 2; i < N; i += 2) {
            w[i] = wr;
            w[i + 1] = wi;
            wtemp = wr;
            wr += wr * wpr - wi * wpi;
            wi += wi * wpr + wtemp * wpi;
        }

        return w;
    }();

    inline static void apply(T* data)
    {
        Next::apply(data);
        Next::apply(data + N);

        SIMD::fft_step(data, w.data(), N);
    }

    inline static void apply_real(T* data)
    {
        apply(data);
    }

    inline static void apply_real_padded(T* data)
    {
        apply (data);
    }
};

template<typename T>
struct DanielsonLanczos<4, T>
{
    inline static void apply(T* data)
    {
        T tr = data[2];
        T ti = data[3];
        data[2] = data[0] - tr;
        data[3] = data[1] - ti;
        data[0] += tr;
        data[1] += ti;
        tr = data[6];
        ti = data[7];
        data[6] = data[5] - ti;
        data[7] = tr - data[4];
        data[4] += tr;
        data[5] += ti;

        tr = data[4];
        ti = data[5];
        data[4] = data[0] - tr;
        data[5] = data[1] - ti;
        data[0] += tr;
        data[1] += ti;
        tr = data[6];
        ti = data[7];
        data[6] = data[2] - tr;
        data[7] = data[3] - ti;
        data[2] += tr;
        data[3] += ti;
    }

    inline static void apply_real(T* data)
    {
        T tr = data[2];
        data[2] = data[0] - tr;
        data[0] += tr;

        tr = data[6];
        data[6] = 0.0f;
        data[7] = tr - data[4];
        data[4] += tr;

        tr = data[4];
        data[4] = data[0] - tr;
        data[0] += tr;

        T ti = data[7];
        data[6] = data[2];
        data[7] = data[3] - ti;
        data[3] += ti;
    }

    inline static void apply_real_padded(T* data)
    {
        data[2] = data[0];

        T tr = data[4];
        data[4] = data[0] - tr;
        data[0] += tr;

        data[6] = data[2];
        data[7] = tr;
        data[3] = -tr;
    }
};

template<typename T>
struct DanielsonLanczos<2, T>
{
    inline static void apply(T* data)
    {
        T tr = data[2];
        T ti = data[3];
        data[2] = data[0] - tr;
        data[3] = data[1] - ti;
        data[0] += tr;
        data[1] += ti;
    }

    inline static void apply_real(T* data)
    {
        const T tr = data[2];
        data[2] = data[0] - tr;
        data[0] += tr;
    }

    inline static void apply_real_padded(T* data)
    {
        data[2] = data[0];
    }
};

template<typename T>
struct DanielsonLanczos<1, T>
{
    static void apply(T* data)
    {
    }
};

template<unsigned N, typename T = float>
struct GFFT
{
    using Impl = DanielsonLanczos<N, T>;

    // Data format: [RIRIRIRI...]
    static void fft(T* data)
    {
        permutate (data);
        Impl::apply (data);
    }

    // Data format: [R0R0R0R0...]
    static void fft_real(T* data)
    {
        permutate_real (data);
        Impl::apply_real (data);
    }

    // Data format: [R0R0R0...000000]
    static void fft_real_padded(T* data)
    {
        permutate_real_padded (data);
        Impl::apply_real_padded (data);
    }

    static void ifft(T* data)
    {
        conj (data);
        fft (data);
        conj (data);

        constexpr float norm = 1.0f / N;

        for (unsigned i = 0; i < 2*N; ++i)
            data[i] *= norm;
    }

    template <typename U>
    static constexpr U log2int(U n)
    {
        return ((n < 2) ? 0 : 1 + log2int(n / 2));
    }

    static void permutate(T* data)
    {
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

    static void permutate_real(T* data)
    {
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

    static void permutate_real_padded(T* data)
    {
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

    static void conj(T* data)
    {
        for (unsigned i = 1; i < 2 * N; i += 2)
            data[i] = -data[i];
    }
};

} // namespace dsp


