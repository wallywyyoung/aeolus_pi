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

#include <complex>
#include <valarray>

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

} // namespace dsp


