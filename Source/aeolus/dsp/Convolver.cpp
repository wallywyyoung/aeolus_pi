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

#include "aeolus/dsp/Convolver.h"
#include "aeolus/SmoothFloat.h"
#include "aeolus/dsp/Convolve.h"

#include "StaticAudioBuffer.h"
#include "aeolus/IR.h"

namespace dsp {

using ConvolverHead = CascadeConvolver<FIR<32>, FFT<32>, FFT<64>, FFT<128>, FFT<256>, FFT<512>, FFT<1024>, FFT<2048>>;

static_assert(ConvolverHead::Length == Convolver::BLOCK_SIZE, "Header block size is wrong");

struct Convolver::Implementation {
    enum State { Idle, Init, FeedHeadIR, ProcessWithIRStream, Process };

    SmoothFloatPool<NUM_PARAMS> params;
    Worker worker;
    size_t length;

    State state;

    ConvolverHead headL;
    ConvolverHead headR;
    bool zeroDelay;

    EquallyPartitionedConvolver<BLOCK_SIZE> convL;
    EquallyPartitionedConvolver<BLOCK_SIZE> convR;

    std::vector<FFT<BLOCK_SIZE>> blocksL{};
    std::vector<FFT<BLOCK_SIZE>> blocksR{};

    // For zero-delay convolution
    StaticAudioBuffer<ConvolverHead::Length, 2> input;
    AudioBuffer ir { 2, ConvolverHead::Length };
    size_t irSamplesRead;

    size_t inputSize;
    size_t framesProcessed;

    explicit Implementation() : length{0} , state{Idle} , zeroDelay{true} , irSamplesRead{0} , inputSize{0} , framesProcessed{0} {
        params[DRY].setName("dry");
        params[DRY].setValue(DEFAULT_DRY, true);

        params[WET].setName("wet");
        params[WET].setValue(DEFAULT_WET, true);

        params[GAIN].setName("gain");
        params[GAIN].setValue(DEFAULT_GAIN, true);

        worker.start();
    }

    ~Implementation() {
        worker.stop();
    }

    void init () {
        const size_t numBlocks = length < BLOCK_SIZE ? 1 : (length - 1) / BLOCK_SIZE + 1;
        inputSize = numBlocks * BLOCK_SIZE;

        convL.resize(numBlocks);
        convR.resize(numBlocks);

        headL.init(ir.getWritePointer(0), input.getWritePointer(0), BLOCK_SIZE);
        headR.init(ir.getWritePointer(1), input.getWritePointer(1), BLOCK_SIZE);

        updateRealtime (false);

        reset();
    }

    void updateRealtime(const bool isNonRealtime) {
        // Run convolution on a side thread for real-time processing.
        if (isNonRealtime) {
            convL.setWorker(nullptr);
            convR.setWorker(nullptr);
        } else {
            convL.setWorker(&worker);
            convR.setWorker(&worker);
        }
    }

    void reset() {
        input.clear();
        ir.clear();
        irSamplesRead = 0;

        headL.reset();
        headR.reset();

        convL.reset();
        convR.reset();

        framesProcessed = 0;
    }

    void setDryWet(const float dry, const float wet, const bool force) {
        params[DRY].setValue(dry, force);
        params[WET].setValue(wet, force);
    }

    [[nodiscard]] bool isAudible() const {
        return params[WET].target() > 0.0f || params[WET].value() > 0.0f;
    }

    void setIR(const IR& newIr) {
        ir = static_cast<AudioBuffer>(newIr);

        // Reset the convolver to the initial state
        irSamplesRead = 0;
        framesProcessed = 0;
        input.clear();

        headL.init(this->ir.getWritePointer(0), input.getWritePointer(0), BLOCK_SIZE);
        headR.init(this->ir.getWritePointer(1), input.getWritePointer(1), BLOCK_SIZE);

        headL.reset();
        headR.reset();

        convL.reset();
        convR.reset();

        int i = zeroDelay ? BLOCK_SIZE : 0;
        const float* irL = ir.getReadPointer(0);
        const float* irR = ir.getReadPointer(1);

        while (i < std::min(static_cast<int>(inputSize), static_cast<int>(ir.getNumSamples()))) {
            convL.feedIr(irL[i]);
            convR.feedIr(irR[i]);
            ++i;
        }

        while (i < inputSize) {
            convL.feedIr(0.0f);
            convR.feedIr(0.0f);
            ++i;
        }

        state = Process;
    }

    void prepareToPlay () {
        state = Init;
        init();
    }

    void process(float *inOut, const size_t framesPerChannel) {
        if (state == Init) {
            state = zeroDelay ? FeedHeadIR : ProcessWithIRStream;
        }

        if (state == FeedHeadIR) {
            // ir buffer is ready at this point
            if (ir.getNumSamples() <= BLOCK_SIZE) {
                irSamplesRead = ir.getNumSamples();
                state = Process;
            } else {
                irSamplesRead = BLOCK_SIZE;
                state = ProcessWithIRStream;
            }
        }

        if (state == ProcessWithIRStream) {
            processFrame(inOut, framesPerChannel);

            framesProcessed += framesPerChannel;

            if (framesProcessed >= inputSize || irSamplesRead >= ir.getNumSamples()) {
                // The entire IR has been read, switch to procesing without IR streaming
                state = Process;
            }

            return;
        }

        if (state == Process) {
            processFrame(inOut, framesPerChannel);
        }
    }

    void processFrame(float *inOut, const size_t framesPerChannel) {
        if (zeroDelay) {
            for (size_t i = 0; i < framesPerChannel; ++i) {
                const float l = convL.tick(inOut[i * 2]) + headL.tick(inOut[i * 2]);
                const float r = convR.tick(inOut[i * 2 + 1]) + headR.tick(inOut[i * 2 + 1]);

                const float dry = params[DRY].nextValue();
                const float wet = params[WET].nextValue();

                inOut[i*2] = l * wet + inOut[i*2] * dry;
                inOut[i*2+1] = r * wet + inOut[i*2+1] * dry;
            }
        } else {
            for (size_t i = 0; i < framesPerChannel; ++i) {
                const float l = convL.tick(inOut[i * 2]);
                const float r = convR.tick(inOut[i * 2 + 1]);

                const float dry = params[DRY].nextValue();
                const float wet = params[WET].nextValue();

                inOut[i*2] = l * wet + inOut[i*2] * dry;
                inOut[i*2+1] = r * wet + inOut[i*2+1] * dry;
            }
        }
    }
};

Convolver::Convolver() : implementation(std::make_unique<Implementation>()) { }

Convolver::~Convolver() = default;

int Convolver::setIR(const IR &ir) {
    implementation->length = static_cast<int>((ir.getNumSamples() / BLOCK_SIZE + 1) * BLOCK_SIZE);
    implementation->setIR(ir);
    implementation->prepareToPlay();
    implementation->zeroDelay = ir.zeroDelay;
    return length();
}

void Convolver::setDryWet(const float dry, const float wet, const bool force) {
    implementation->setDryWet(dry, wet, force);
}

bool Convolver::isAudible() const {
    return implementation->isAudible();
}

void Convolver::process(float *inOut, const size_t framesPerChannel, const bool nonRealtime) const {
    implementation->updateRealtime(nonRealtime);
    implementation->process(inOut, framesPerChannel);
}

int Convolver::length() const noexcept
{
    return static_cast<int>(implementation->length);
}
} // namespace dsp


