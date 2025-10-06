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

#include "aeolus/dsp/Convolution/Convolver.h"

#include <future>

int Convolver::setIR(const IR &newIr) {
    state = INIT;

    ir = newIr;

    // Set Length
    length = (ir.getNumSamples() / BLOCK_SIZE + 1) * BLOCK_SIZE;
    // Prepare to Play
    const size_t numBlocks = length < BLOCK_SIZE ? 1 : (length - 1) / BLOCK_SIZE + 1;
    inputSize = numBlocks * BLOCK_SIZE;

    // Reset the convolver to the initial state
    irSamplesRead = 0;
    framesProcessed = 0;
    input.clear();

    // uniformPartitionedL.resizeAndReset(numBlocks);
    // uniformPartitionedR.resizeAndReset(numBlocks);
    epcL.resize(numBlocks);
    epcR.resize(numBlocks);
    epcL.reset();
    epcR.reset();

    cascadeL.init(ir.getWritePointer (0), input.getWritePointer (0));
    cascadeR.init(ir.getWritePointer (1), input.getWritePointer (1));

    int i = BLOCK_SIZE;
    const float* irL = ir.getReadPointer(0);
    const float* irR = ir.getReadPointer(1);
    while (i < std::min(inputSize, newIr.getNumSamples())) {
        // uniformPartitionedL.feedIr(irL[i]);
        // uniformPartitionedR.feedIr(irR[i]);
        epcL.feedIr(irL[i]);
        epcR.feedIr(irR[i]);
        ++i;
    }
    while (i < inputSize) {
        // uniformPartitionedL.feedIr(0.0f);
        // uniformPartitionedR.feedIr(0.0f);
        epcL.feedIr(0.0f);
        epcR.feedIr(0.0f);
        ++i;
    }

    state = PROCESS;

    return getLength();
}

void Convolver::process(float *inOut, const size_t framesPerChannel) {
    if (state == IDLE) {
        return;
    }

    if (state == PROCESS_WITH_IR_STREAM || state == PROCESS) {
        for (auto i = 0; i < framesPerChannel; ++i) {
            const auto l = epcL.tick(inOut[i * 2]) + cascadeL.tick(inOut[i * 2]);
            const auto r = epcR.tick(inOut[i * 2 + 1]) + cascadeR.tick(inOut[i * 2 + 1]);
            const auto thisDry = dry.nextValue();
            const auto thisWet = wet.nextValue();
            inOut[i*2] = (l * thisWet) + (inOut[i*2] * thisDry);
            inOut[i*2+1] = (r * thisWet) + (inOut[i*2+1] * thisDry);
        }
    }

    if (state == PROCESS_WITH_IR_STREAM) {
        framesProcessed += framesPerChannel;
        if (framesProcessed >= inputSize || irSamplesRead >= ir.getNumSamples()) {
            // The entire IR has been read, switch to processing without IR streaming
            state = PROCESS;
        }
    }
}
