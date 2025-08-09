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

#include "aeolus/SIMD.h"
#include <arm_neon.h>
#include <numbers>

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
        for (unsigned long i = 0; i < size; ++i) {
            sum += x[i] * y[i];
        }
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

    // https://github.com/ARM-software/EndpointAI/blob/master/Kernels/Migrating_to_Helium_from_Neon_Companion_SW/vmath.c
    float32x4_t vsinq_neon_f32(float32x4_t val) {
        auto pi = std::numbers::pi_v<float>;
        constexpr float te_sin_coeff2 = 0.166666666666f;    // 1/(2*3)
        constexpr float te_sin_coeff3 = 0.05f;              // 1/(4*5)
        constexpr float te_sin_coeff4 = 0.023809523810f;    // 1/(6*7)
        constexpr float te_sin_coeff5 = 0.013888888889f;    // 1/(8*9)

        const float32x4_t pi_v = vdupq_n_f32(pi);
        const float32x4_t pio2_v = vdupq_n_f32(pi / 2);
        const float32x4_t ipi_v = vdupq_n_f32(1 / pi);

        //Find positive or negative
        const int32x4_t c_v = vabsq_s32(vcvtq_s32_f32(vmulq_f32(val, ipi_v)));
        const uint32x4_t sign_v = vcleq_f32(val, vdupq_n_f32(0));
        const uint32x4_t odd_v = vandq_u32(vreinterpretq_u32_s32(c_v), vdupq_n_u32(1));

        uint32x4_t      neg_v = veorq_u32(odd_v, sign_v);

        //Modulus a - (n * int(a*(1/n)))
        float32x4_t     ma = vsubq_f32(vabsq_f32(val), vmulq_f32(pi_v, vcvtq_f32_s32(c_v)));

        const uint32x4_t reb_v = vcgeq_f32(ma, pio2_v);

        //Rebase a between 0 and pi/2
        ma = vbslq_f32(reb_v, vsubq_f32(pi_v, ma), ma);

        //Taylor series
        const float32x4_t ma2 = vmulq_f32(ma, ma);

        //2nd elem: x^3 / 3!
        float32x4_t elem = vmulq_f32(vmulq_f32(ma, ma2), vdupq_n_f32(te_sin_coeff2));
        float32x4_t res = vsubq_f32(ma, elem);

        //3rd elem: x^5 / 5!
        elem = vmulq_f32(vmulq_f32(elem, ma2), vdupq_n_f32(te_sin_coeff3));
        res = vaddq_f32(res, elem);

        //4th elem: x^7 / 7!float32x2_t vsin_f32(float32x2_t val)
        elem = vmulq_f32(vmulq_f32(elem, ma2), vdupq_n_f32(te_sin_coeff4));
        res = vsubq_f32(res, elem);

        //5th elem: x^9 / 9!
        elem = vmulq_f32(vmulq_f32(elem, ma2), vdupq_n_f32(te_sin_coeff5));
        res = vaddq_f32(res, elem);

        //Change of sign
        neg_v = vshlq_n_u32(neg_v, 31);
        res = vreinterpretq_f32_u32(veorq_u32(vreinterpretq_u32_f32(res), neg_v));
        return res;
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