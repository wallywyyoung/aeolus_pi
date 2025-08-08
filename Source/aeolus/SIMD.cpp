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

// TODO: Add NEON128
namespace no_simd {
    void add(float* out, const float* in, const unsigned long size) {
        for (unsigned long i = 0; i < size; ++i) {
            out[i] += in[i];
        }
    }

    void mul_const_add(float* out, const float* in, const float k, const unsigned long size) {
        for (unsigned long i = 0; i < size; ++i)
            out[i] += in[i] * k;
    }

    void add_mul_const(float* out, const float* in, const float k, const unsigned long size) {
        for (unsigned long i = 0; i < size; ++i)
            out[i] = k * (out[i] + in[i]);
    }

    void mul_const(float* out, const float k, const unsigned long size) {
        for (unsigned long i = 0; i < size; ++i)
            out[i] *= k;
    }

    float mul_reduce(const float* x, const float* y, const unsigned long size) {
        float sum = 0.0f;

        for (unsigned long i = 0; i < size; ++i)
            sum += x[i] * y[i];

        return sum;
    }

    void complex_mul(float* res, const float* a, const float* b, const unsigned long size) {
        for (unsigned long i = 0; i < size; i += 2) {
            res[i] = a[i] * b[i] - a[i + 1]* b[i + 1];
            res[i + 1] = a[i] * b[i + 1] + a[i + 1] * b[i];
        }
    }

    void complex_mul_conj(float* res, const float* a, const float* b, const unsigned long size) {
        for (unsigned long i = 0; i < size; i += 2) {
            const float x = a[i] * b[i] - a[i + 1] * b[i + 1];
            const float y = -a[i] * b[i + 1] - a[i + 1] * b[i];
            res[i] = x;
            res[i + 1] = y;
        }
    }

    void fft_step(float* data, const float* w, const unsigned long n) {
        for (unsigned i = 0; i < n; i += 2) {
            const float tempr = data[i + n] * w[i] - data[i + n + 1] * w[i + 1];
            const float tempi = data[i + n] * w[i + 1] + data[i + n + 1] * w[i];
            data[i + n] = data[i] - tempr;
            data[i + n + 1] = data[i + 1] - tempi;
            data[i] += tempr;
            data[i + 1] += tempi;
        }
    }
}

void  (*SIMD::add)(float*, const float*, unsigned long)                            = &no_simd::add;
void  (*SIMD::mul_const_add)(float*, const float*, const float, unsigned long)     = &no_simd::mul_const_add;
void  (*SIMD::add_mul_const)(float*, const float*, const float, unsigned long)     = &no_simd::add_mul_const;
void  (*SIMD::mul_const)(float*, const float, unsigned long)                       = &no_simd::mul_const;
float (*SIMD::mul_reduce)(const float*, const float*, unsigned long)               = &no_simd::mul_reduce;
float (*SIMD::mul_reduce_unaligned)(const float*, const float*, unsigned long)     = &no_simd::mul_reduce;
void  (*SIMD::complex_mul)(float*, const float*, const float*, unsigned long)      = &no_simd::complex_mul;
void  (*SIMD::complex_mul_conj)(float*, const float*, const float*, unsigned long) = &no_simd::complex_mul_conj;
void  (*SIMD::fft_step)(float*, const float*, unsigned long)                       = &no_simd::fft_step;