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

#include "aeolus/audioparam.h"
#include "aeolus/dsp/convolve.h"
#include "aeolus/dsp/convolver.h"
#include "aeolus/IR.h"

namespace dsp {

using ConvHead = CascadeConvolver<FIR<32>, FFT<32>, FFT<64>, FFT<128>, FFT<256>, FFT<512>, FFT<1024>, FFT<2048>>;

static_assert(ConvHead::Length == Convolver::BlockSize, "Header block size is wrong");

struct Convolver::Impl {
    enum State {
        Idle,
        Init,
        FeedHeadIR,
        ProcessWithIRStream,
        Process
    };

    AudioParameterPool params;
    Worker worker;
    size_t length;

    State state;

    ConvHead headL;
    ConvHead headR;
    bool zeroDelay;

    EquallyPartitionedConvolver<BlockSize> convL;
    EquallyPartitionedConvolver<BlockSize> convR;

    std::vector<FFT<BlockSize>> blocksL{};
    std::vector<FFT<BlockSize>> blocksR{};

    // For zero-delay convolution
    AudioBuffer input;
    AudioBuffer ir;
    size_t irSamplesRead;

    size_t inputSize;
    size_t framesProcessed;

    Impl () : params{NUM_PARAMS} , length{0} , state{Idle} , zeroDelay{true} , input(2, ConvHead::Lenght) , ir(2, ConvHead::Lenght) , irSamplesRead{0} , inputSize{0} , framesProcessed{0} {
        params[DRY].setName("dry");
        params[DRY].setValue(DefaultDry, true);

        params[WET].setName("wet");
        params[WET].setValue(DefaultWet, true);

        params[GAIN].setName("gain");
        params[GAIN].setValue(DefaultGain, true);

        worker.start();
    }

    ~Impl()
    {
        worker.stop();
    }

    void init ()
    {
        const size_t numBlocks = length < BlockSize ? 1 : (length - 1) / BlockSize + 1;
        inputSize = numBlocks * BlockSize;

        convL.resize(numBlocks);
        convR.resize(numBlocks);

        headL.init(ir.getWritePointer(0), input.getWritePointer(0), BlockSize);
        headR.init(ir.getWritePointer(1), input.getWritePointer(1), BlockSize);

        updateRealtime (false);

        reset();
    }

    void updateRealtime(const bool isNonRealtime)
    {
        // Run convolution on a side thread for real-time processing.
        if (isNonRealtime) {
            convL.setWorker(nullptr);
            convR.setWorker(nullptr);
        } else {
            convL.setWorker(&worker);
            convR.setWorker(&worker);
        }
    }

    void reset()
    {
        input.clear();
        ir.clear();
        irSamplesRead = 0;

        headL.reset();
        headR.reset();

        convL.reset();
        convR.reset();

        framesProcessed = 0;
    }

    void setDryWet(const float dry, const float wet, const bool force)
    {
        params[DRY].setValue(dry, force);
        params[WET].setValue(wet, force);
    }

    float isAudible() const
    {
        return params[WET].target() > 0.0f
            || params[WET].value() > 0.0f;
    }

    void setIR(const IR& newIr)
    {
        ir = static_cast<AudioBuffer>(newIr);

        // Reset the convolver to the initial state
        irSamplesRead = 0;
        framesProcessed = 0;
        input.clear();

        headL.init(this->ir.getWritePointer(0), input.getWritePointer(0), BlockSize);
        headR.init(this->ir.getWritePointer(1), input.getWritePointer(1), BlockSize);

        headL.reset();
        headR.reset();

        convL.reset();
        convR.reset();

        int i = zeroDelay ? BlockSize : 0;
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

    void prepareToPlay ()
    {
        state = Init;
        init();
    }

    void process(const float *inL, const float *inR, float *outL, float *outR, const size_t numFrames)
    {
        if (state == Init)
            state = zeroDelay ? FeedHeadIR : ProcessWithIRStream;

        if (state == FeedHeadIR)
        {
            // ir buffer is ready at this point
            if (ir.getNumSamples() <= BlockSize) {
                irSamplesRead = ir.getNumSamples();
                state = Process;
            } else {
                irSamplesRead = BlockSize;
                state = ProcessWithIRStream;
            }
        }

        if (state == ProcessWithIRStream) {
            processFrame(inL, inR, outL, outR, numFrames);

            framesProcessed += numFrames;

            if (framesProcessed >= inputSize || irSamplesRead >= ir.getNumSamples()) {
                // The entire IR has been read, switch to procesing without IR streaming
                state = Process;
            }

            return;
        }

        if (state == Process) {
            processFrame(inL, inR, outL, outR, numFrames);
        }
    }

    void processFrame(const float *inL, const float *inR, float *outL, float *outR, const size_t numFrames)
    {
        if (zeroDelay) {
            for (size_t i = 0; i < numFrames; ++i) {
                const float l = convL.tick(inL[i]) + headL.tick(inL[i]);
                const float r = convR.tick(inR[i]) + headR.tick(inR[i]);

                const float dry = params[DRY].nextValue();
                const float wet = params[WET].nextValue();

                outL[i] = l * wet + inL[i] * dry;
                outR[i] = r * wet + inR[i] * dry;
            }
        } else {
            for (size_t i = 0; i < numFrames; ++i) {
                const float l = convL.tick(inL[i]);
                const float r = convR.tick(inR[i]);

                const float dry = params[DRY].nextValue();
                const float wet = params[WET].nextValue();

                outL[i] = l * wet + inL[i] * dry;
                outR[i] = r * wet + inR[i] * dry;
            }
        }
    }
};

//----------------------------------------------------------

Convolver::Convolver()
    : d(std::make_unique<Impl>())
{
}

Convolver::~Convolver() = default;

void Convolver::setIR(const IR& ir)
{
    d->setIR(ir);
}

void Convolver::setDryWet(const float dry, const float wet, const bool force)
{
    d->setDryWet(dry, wet, force);
}

bool Convolver::isAudible() const
{
    return d->isAudible();
}

void Convolver::prepareToPlay()
{
    d->prepareToPlay();
}

void Convolver::process(const float *inL, const float *inR, float *outL, float *outR, const size_t numFrames) const {
    d->process(inL, inR, outL, outR, numFrames);
}

void Convolver::setNonRealtime(const bool nonRealtime) const {
    d->updateRealtime(nonRealtime);
}

int Convolver::length() const noexcept
{
    return static_cast<int>(d->length);
}

void Convolver::setLength(const int len) const noexcept
{
    d->length = len;
}

bool Convolver::zeroDelay() const noexcept
{
    return d->zeroDelay;
}

void Convolver::setZeroDelay(const bool v) const noexcept
{
    d->zeroDelay = v;
}

} // namespace dsp


