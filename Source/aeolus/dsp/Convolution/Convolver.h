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

#include "StaticAudioBuffer.h"
#include "aeolus/IR.h"
#include "aeolus/SmoothFloat.h"
#include "aeolus/Worker.h"
#include "aeolus/dsp/Convolution/CascadeConvolver.h"
#include "aeolus/dsp/Convolution/EquallyPartitionedConvolver.h"

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
    EquallyPartitionedConvolver<BLOCK_SIZE> epcL{};
    EquallyPartitionedConvolver<BLOCK_SIZE> epcR{};

    // For zero-delay convolution
    StaticAudioBuffer<CascadeConvolver::BLOCK_SIZE, 2> input{};
    IR ir{"", AudioFile<float>{}};
    size_t irSamplesRead{ 0 };
    size_t inputSize{ 0 };
    size_t framesProcessed{ 0 };
    size_t tailCounter{ 0 };

    [[nodiscard]] bool isAudible() const {
        return wet.target() > 0.0f || wet.value() > 0.0f;
    }

public:
    Convolver() = default;
    ~Convolver() = default;

    void setIR(const IR &newIr);

    void setDryWet(const float newDry, const float newWet, const bool force = false) {
        dry.setValue(newDry, force);
        wet.setValue(newWet, force);
    }

    template<size_t SAMPLES_PER_CHANNEL>
    void process(float *inOut, const bool& wasAudioGenerated) {
        // When there is no audio generated, we let the reverb tail sound and stop the reverb processing to avoid convolving with silence.
        tailCounter = wasAudioGenerated ? length : std::max(0, static_cast<int>(tailCounter) - static_cast<int>(SAMPLES_PER_CHANNEL));
        if (state == IDLE || !isAudible() || tailCounter == 0) {
            return;
        }

        if (state == PROCESS_WITH_IR_STREAM || state == PROCESS) {
            for (auto i = 0; i < SAMPLES_PER_CHANNEL; ++i) {
                const auto l = epcL.tick(inOut[i * 2]) + cascadeL.tick(inOut[i * 2]);
                const auto r = epcR.tick(inOut[i * 2 + 1]) + cascadeR.tick(inOut[i * 2 + 1]);
                const auto thisDry = dry.nextValue();
                const auto thisWet = wet.nextValue();
                inOut[i*2] = (l * thisWet) + (inOut[i*2] * thisDry);
                inOut[i*2+1] = (r * thisWet) + (inOut[i*2+1] * thisDry);
            }
        }

        if (state == PROCESS_WITH_IR_STREAM) {
            framesProcessed += SAMPLES_PER_CHANNEL;
            if (framesProcessed >= inputSize || irSamplesRead >= ir.getNumSamples()) {
                // The entire IR has been read, switch to processing without IR streaming
                state = PROCESS;
            }
        }
    }
};
