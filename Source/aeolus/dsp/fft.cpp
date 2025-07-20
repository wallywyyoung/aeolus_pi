// ----------------------------------------------------------------------------
//
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

#include "aeolus/dsp/fft.h"

#include <cmath>
#include <algorithm>





namespace dsp {

static float hann(const int i, const int n)
{
    return 0.5f * (1.0f - std::cos (M_PI * 2.0f * i / (n - 1)));
}

static float hamming(const int i, const int n)
{
    return 0.53836f + 0.46164f * std::cos (M_PI * 2.0f * i / (n - 1));
}

static float blackman(const int i, const int n)
{
    const auto x = M_PI * 2.0f * i / (n - 1);
    return 0.42659f - 0.49656f * std::cos (x) + 0.076849f * std::cos (2.0f * x);
}

void Fft::direct(Array& x, const Window win)
{
    applyWindow(x, win);

    const auto N = static_cast<unsigned int>(x.size());
    // DFT
    unsigned int k = N;
    const float thetaT = M_PI / N;
    Complex phiT = Complex (std::cos (thetaT), std::sin (thetaT)), T;

    while (k > 1) {
        const unsigned int n = k;
        k >>= 1;
        phiT = phiT * phiT;
        T = 1.0L;

        for (unsigned int l = 0; l < k; l++) {
            for (unsigned int a = l; a < N; a += n) {
                const unsigned int b = a + k;
                Complex t = x[a] - x[b];
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
            const Complex t = x[a];
            x[a] = x[b];
            x[b] = t;
        }
    }

}

void Fft::inverse(Array &x)
{
    // conjugate the complex numbers
    x = x.apply(std::conj);

    // forward fft
    direct (x);

    // conjugate the complex numbers again
    x = x.apply(std::conj);

    // scale the numbers
    std::ranges::transform(begin(x),end(x), begin(x),[x](const Complex c){return c / static_cast<float>(x.size());});
    // x /= static_cast<float>(x.size());
}

void Fft::applyWindow(Array&x, const Window win)
{
    switch (win) {
    case Window::None:
        break;
    case Window::Hann:
        for (int i = 0; i < x.size(); ++i)
            x[i] *= hann(i, static_cast<int>(x.size()));
        break;
    case Window::Hamming:
        for (int i = 0; i < x.size(); ++i)
            x[i] *= hamming(i, static_cast<int>(x.size()));
        break;
    case Window::Blackman:
        for (int i = 0; i < x.size(); ++i)
            x[i] *= blackman(i, static_cast<int>(x.size()));
        break;
    default:
        break;
    }
}

} // namespace dsp


