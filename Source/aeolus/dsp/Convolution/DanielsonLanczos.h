#pragma once

#include <array>
#include "aeolus/SIMD.h"
#include "aeolus/globals.h"

template<unsigned N, typename T = float>
struct DanielsonLanczos;

template<unsigned N, typename T>
struct DanielsonLanczos {
    using Next = DanielsonLanczos<N / 2, T>;

    alignas(32) inline static std::array<float, N> w = [] {
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

    static void apply(T* data)
    {
        Next::apply(data);
        Next::apply(data + N);

        SIMD::fft_step(data, w.data(), N);
    }

    static void apply_real(T* data)
    {
        apply(data);
    }

    static void apply_real_padded(T* data)
    {
        apply (data);
    }
};

template<typename T>
struct DanielsonLanczos<4, T> {
    static void apply(T* data)
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

    static void apply_real(T* data)
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

    static void apply_real_padded(T* data)
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
struct DanielsonLanczos<2, T> {
    static void apply(T* data)
    {
        T tr = data[2];
        T ti = data[3];
        data[2] = data[0] - tr;
        data[3] = data[1] - ti;
        data[0] += tr;
        data[1] += ti;
    }

    static void apply_real(T* data)
    {
        const T tr = data[2];
        data[2] = data[0] - tr;
        data[0] += tr;
    }

    static void apply_real_padded(T* data)
    {
        data[2] = data[0];
    }
};

template<typename T>
struct DanielsonLanczos<1, T> {
    static void apply(T*) { }
};
