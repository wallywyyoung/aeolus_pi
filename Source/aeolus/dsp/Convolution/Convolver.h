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

#include "../../../../EquallyPartitionedConvolver.h"
#include "StaticAudioBuffer.h"
#include "aeolus/IR.h"
#include "aeolus/SmoothFloat.h"
#include "aeolus/Worker.h"
#include "aeolus/dsp/Convolution/CascadeConvolver.h"
#include "aeolus/dsp/Convolution/UniformPartitionedConvolver.h"

/**
 * @brief Stereo convolution reverb.
 */
class Convolver final {
public:
    constexpr static size_t BLOCK_SIZE = 4096; ///< Single convolution block size (in number of samples).
    static_assert(CascadeConvolver::BLOCK_SIZE == BLOCK_SIZE, "Header block size is wrong");
private:
    enum State { IDLE, INIT, FEED_HEAD_IR, PROCESS_WITH_IR_STREAM, PROCESS };

    std::shared_ptr<Worker> worker{ std::make_shared<Worker>() };

    SmoothFloat dry { 1.0f };
    SmoothFloat wet { 0.25f };
    SmoothFloat irSampleGain { 1.0f };

    size_t length{ 0 };
    State state{ IDLE };

    CascadeConvolver cascadeL{};
    CascadeConvolver cascadeR{};

    // UniformPartitionedConvolver<BLOCK_SIZE> uniformPartitionedL{};
    // UniformPartitionedConvolver<BLOCK_SIZE> uniformPartitionedR{};
    EquallyPartitionedConvolver<BLOCK_SIZE> epcL{};
    EquallyPartitionedConvolver<BLOCK_SIZE> epcR{};

    // For zero-delay convolution
    StaticAudioBuffer<CascadeConvolver::BLOCK_SIZE, 2> input{};
    IR ir{"", AudioFile<float>{}};
    size_t irSamplesRead{ 0 };
    size_t inputSize{ 0 };
    size_t framesProcessed{ 0 };

public:
    Convolver() = default;
    ~Convolver() = default;

    [[nodiscard]] int setIR(const IR &newIr);

    void setDryWet(const float newDry, const float newWet, const bool force = false) {
        dry.setValue(newDry, force);
        wet.setValue(newWet, force);
    }

    [[nodiscard]] bool isAudible() const {
        return wet.target() > 0.0f || wet.value() > 0.0f;
    }

    void process(float *inOut, size_t framesPerChannel);

    [[nodiscard]] int getLength() const noexcept { return static_cast<int>(length); }
};
