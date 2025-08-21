// ----------------------------------------------------------------------------
//
//  Copyright (C) 2025 Wally Young <wallywyyoung@users.noreply.github.com>
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

#include <cstdint>

#include "MemoryConstants.h"

class SimdUtilities {
public:
    // Using inline assembly for ARMv8-a enabling flush-to-zero
    // Denormals are handled differently in ARMv8-a so there is no equivalent denormals-are-zero
    static void enableFlushToZero();
    static void disableFlushToZero();
    // Ensure your in and out buffers are aligned to 16 to be NEON compliant.
    static void ConvertF32toS16(float (&in)[NUMBER_SAMPLES], std::int16_t* out);
    // Ensure your in and out buffers are aligned to 16 to be NEON compliant.
    static void ConvertF32toS24(float (&in)[NUMBER_SAMPLES], std::uint8_t* out);
};
