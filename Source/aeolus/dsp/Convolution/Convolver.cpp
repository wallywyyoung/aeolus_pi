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

void Convolver::setIR(const IR &newIr) {
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
        epcL.feedIr(irL[i]);
        epcR.feedIr(irR[i]);
        ++i;
    }
    while (i < inputSize) {
        epcL.feedIr(0.0f);
        epcR.feedIr(0.0f);
        ++i;
    }

    state = PROCESS;
}
