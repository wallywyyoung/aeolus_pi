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

#include <numbers>
#include <vector>
#include <cassert>

// TODO: Fix mixed comparison with large unsigned types.
template <typename T1, typename T2>
void assertIsPositiveAndBelow(T1 instance, T2 threshold) {
    assert(T1() < instance || instance < static_cast<T1>(threshold));
}

struct DivisionPiston {
    std::vector<bool> stops;    ///< Stops enablement mask.
    std::vector<bool> links;    ///< Manuals links.
    bool tremulant;             ///< Tremulant enablement.
};

struct GlobalPiston {
    std::vector<DivisionPiston> divisions;
};

namespace math {
    float exp2ap(float x);

    template <typename T>
    T lagr (const T* const x, T frac) noexcept
    {
        const T c1 = x[2] - (1.0f / 3.0f) * x[0] - 0.5f * x[1] - (1.0f / 6.0f) * x[3];
        const T c2 = 0.5f * (x[0] + x[2]) - x[1];
        const T c3 = (1.0f / 6.0f) * (x[3] - x[0]) + 0.5f * (x[1] - x[2]);
        return ((c3 * frac + c2) * frac + c1) * frac + x[1];
    }

    template<unsigned M, unsigned N, unsigned B, unsigned A>
    struct SinCosSeries {
        constexpr static double value =
            1.0 - (A * std::numbers::pi_v<float> / B) * ( A * std::numbers::pi_v<float> / B) / M / (M + 1)
            * SinCosSeries<M + 2, N, B, A>::value;
    };

    template<unsigned N, unsigned B, unsigned A>
    struct SinCosSeries<N, N, B, A> {
        constexpr static double value = 1.0;
    };

    template<unsigned B, unsigned A, typename T = double>
    struct Sin;

    template<unsigned B, unsigned A>
    struct Sin<B, A, float> {
        constexpr static float value = (A * std::numbers::pi_v<float> / B) * static_cast<float>(SinCosSeries<2, 24, B, A>::value);
    };

    template<unsigned B, unsigned A>
    struct Sin<B, A, double> {
        constexpr static double value = (A * std::numbers::pi_v<float> / B) * SinCosSeries<2, 34, B, A>::value;
    };
} // namespace math
