//
// Created by Wally Young on 11/22/25.
//

#pragma once

#include "StereoFIRConvolver.h"
#include "StereoPartitionedConvolver.h"
#include "aeolus/IR.h"

class Convolver {
    StereoPartitionedConvolver stereoPartitionedConvolver;
    StereoFIRConvolver stereoFirConvolver;
    std::array<float, PROCESS_FRAMES_SIZE> leftFir{}, rightFir{};

public:
    void init(const IR& ir) {
        stereoFirConvolver.init(ir);
        stereoPartitionedConvolver.init(ir);
    }

    void process(float *left, float *right) {
        stereoFirConvolver.process(left, right, leftFir.data(), rightFir.data());
        stereoPartitionedConvolver.process(left, right, leftFir.data(), rightFir.data());
    }
};
