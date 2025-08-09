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

#pragma once

#include "aeolus/IR.h"
#include <memory>



namespace dsp {

/**
 * @brief Stereo convolution reverb.
 */
class Convolver final
{
public:

    enum Params
    {
        DRY = 0,
        WET,
        GAIN,   // IR sample gain.

        NUM_PARAMS
    };

    // Default parameters set on creation
    constexpr static float DefaultDry  = 1.0f;
    constexpr static float DefaultWet  = 0.25f;
    constexpr static float DefaultGain = 1.0f;

    /// Single convolution block size (in number of samples).
    constexpr static size_t BlockSize = 4096;

    Convolver();

    ~Convolver();

    int setIR(const IR &ir);

    void setDryWet(float dry, float wet, bool force = false);

    bool isAudible() const;

    void process(float *inOut, size_t framesPerChannel, bool nonRealtime = false) const;

    int length() const noexcept;

protected:

    struct Impl;
    std::unique_ptr<Impl> d;
};

} // namespace dsp


