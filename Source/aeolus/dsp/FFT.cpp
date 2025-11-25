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

#include "aeolus/dsp/FFT.h"
#include <arm_math.h>
#include <numbers>

namespace dsp {

static float hann(const int i, const int n) { return 0.5f * (1.0f - arm_cos_f32(std::numbers::pi_v<float> * 2.0f * static_cast<float>(i) / static_cast<float>(n - 1))); }

static float hamming(const int i, const int n) { return 0.53836f + 0.46164f * arm_cos_f32(std::numbers::pi_v<float> * 2.0f * static_cast<float>(i) / static_cast<float>(n - 1)); }

static float blackman(const int i, const int n) {
    const auto x = std::numbers::pi_v<float> * 2.0f * static_cast<float>(i) / static_cast<float>(n - 1);
    return 0.42659f - 0.49656f * arm_cos_f32(x) + 0.076849f * arm_cos_f32(2.0f * x);
}

void Fft::direct(std::valarray<std::complex<float>>& x, const Window win) {
    applyWindow(x, win);
    // DFT
    const auto N = static_cast<unsigned int>(x.size());
    auto k = N;
    const auto thetaT = std::numbers::pi_v<float> / static_cast<float>(N);
    auto phiT = std::complex(arm_cos_f32(thetaT), arm_sin_f32(thetaT));
	// TODO T and n WERE DEFINED HERE
    while (k > 1) {
        const unsigned int n = k; // TODO
        k >>= 1;
        phiT = phiT * phiT;
        std::complex T{1.0f, 0.0f}; // TODO

        for (unsigned int l = 0; l < k; l++) {
            for (unsigned int a = l; a < N; a += n) {
                const auto b = a + k;
                auto t = x[a] - x[b];
                x[a] += x[b];
                x[b] = t * T;
            }

            T *= phiT;
        }
    }

    // Decimate
    const auto m = static_cast<unsigned int>(log2(N));

    for (unsigned int a = 0; a < N; a++) {
        unsigned int b = a;
        // Reverse bits
        b = (((b & 0xaaaaaaaa) >> 1) | ((b & 0x55555555) << 1));
        b = (((b & 0xcccccccc) >> 2) | ((b & 0x33333333) << 2));
        b = (((b & 0xf0f0f0f0) >> 4) | ((b & 0x0f0f0f0f) << 4));
        b = (((b & 0xff00ff00) >> 8) | ((b & 0x00ff00ff) << 8));
        b = ((b >> 16) | (b << 16)) >> (32 - m);

        if (b > a) {
            const auto t = x[a];
            x[a] = x[b];
            x[b] = t;
        }
    }

}

void Fft::inverse(std::valarray<std::complex<float>> &x) {
    // conjugate the complex numbers
    x = x.apply(std::conj);

    // forward fft
    direct (x);

    // conjugate the complex numbers again
    x = x.apply(std::conj);

    // scale the numbers

    x /= static_cast<float>(x.size());
}

void Fft::applyWindow(std::valarray<std::complex<float>>&x, const Window win) {
    switch (win) {
    case Window::None:
        break;
    case Window::Hann:
        for (auto i = 0; i < x.size(); ++i)
            x[i] *= hann(i, static_cast<int>(x.size()));
        break;
    case Window::Hamming:
        for (auto i = 0; i < x.size(); ++i)
            x[i] *= hamming(i, static_cast<int>(x.size()));
        break;
    case Window::Blackman:
        for (auto i = 0; i < x.size(); ++i)
            x[i] *= blackman(i, static_cast<int>(x.size()));
        break;
    default:
        break;
    }
}

} // namespace dsp


