//
// Created by Wally Young on 11/23/25.
//

#include "StereoPartitionedConvolver.h"
#include <algorithm>
#include "StereoFIRConvolver.h"

void StereoPartitionedConvolver::init(const IR& ir) {
    irSize = ir.getNumberFrames() - StereoFIRConvolver::NUMBER_TAPS;
    numberPartitions = (irSize + PARTITION_SIZE - 1) / PARTITION_SIZE;
    leftInputSpectrumHistory.resize(numberPartitions);
    rightInputSpectrumHistory.resize(numberPartitions);
    leftIRSpectrum.resize(numberPartitions);
    rightIRSpectrum.resize(numberPartitions);
    arm_rfft_fast_init_f32(&leftFFT, PARTITION_SIZE * 2);
    arm_rfft_fast_init_f32(&rightFFT, PARTITION_SIZE * 2);
    std::array<float, PARTITION_SIZE * 2> leftTemp{}, rightTemp{};
    const float* left = ir.getReadPointer(0, StereoFIRConvolver::NUMBER_TAPS);
    const float* right = ir.getReadPointer(1, StereoFIRConvolver::NUMBER_TAPS);
    for (auto p = 0; p < numberPartitions; ++p) {
        const auto offset = p * PARTITION_SIZE;
        const auto partitionSamples = std::min(irSize - offset, PARTITION_SIZE);
        std::copy_n(left + offset, partitionSamples,leftTemp.begin());
        std::copy_n(right + offset,partitionSamples, rightTemp.begin());
        std::fill(leftTemp.begin() + partitionSamples, leftTemp.end(), 0.0f);
        std::fill(rightTemp.begin() + partitionSamples, rightTemp.end(), 0.0f);
        arm_rfft_fast_f32(&leftFFT,leftTemp.data(), leftIRSpectrum[p].data(), 0);
        arm_rfft_fast_f32(&rightFFT, rightTemp.data(), rightIRSpectrum[p].data(), 0);
    }
}

void StereoPartitionedConvolver::process(float* left, float* right, const float* firLeft, const float* firRight) {
    std::array<float, PARTITION_SIZE * 2> leftTemp{}, rightTemp{}, leftAccumulated{}, rightAccumulated{};
    std::copy_n(left, PARTITION_SIZE, leftTemp.begin());
    std::copy_n(right, PARTITION_SIZE, rightTemp.begin());
    std::fill(leftTemp.begin() + PARTITION_SIZE, leftTemp.end(), 0.0f);
    std::fill(rightTemp.begin() + PARTITION_SIZE, rightTemp.end(), 0.0f);
    arm_rfft_fast_f32(&leftFFT, leftTemp.data(), leftBuffer.data(), 0);
    arm_rfft_fast_f32(&rightFFT, rightTemp.data(), rightBuffer.data(), 0);
    leftInputSpectrumHistory[historyIndex] = leftBuffer;
    rightInputSpectrumHistory[historyIndex] = rightBuffer;
    for (auto p = 0; p < numberPartitions; ++p) {
        auto nHistoryAgoIndex = (historyIndex + numberPartitions - p) % numberPartitions;
        arm_cmplx_mult_cmplx_f32(leftInputSpectrumHistory[nHistoryAgoIndex].data(), leftIRSpectrum[p].data(), leftTemp.data(), PARTITION_SIZE);
        arm_cmplx_mult_cmplx_f32(rightInputSpectrumHistory[nHistoryAgoIndex].data(), rightIRSpectrum[p].data(), rightTemp.data(), PARTITION_SIZE);
        for (auto i = 0; i < PARTITION_SIZE * 2; ++i) {
            leftAccumulated[i] += leftTemp[i];
            rightAccumulated[i] += rightTemp[i];
        }
    }
    std::array<float, PARTITION_SIZE * 2> leftTimeDomain{}, rightTimeDomain{};
    arm_rfft_fast_f32(&leftFFT, leftAccumulated.data(), leftTimeDomain.data(), 1);
    arm_rfft_fast_f32(&rightFFT, rightAccumulated.data(), rightTimeDomain.data(), 1);
    constexpr static float NORM = 1.0f / (PARTITION_SIZE * 2);
    for (auto i = 0; i < PARTITION_SIZE; ++i) {
        left[i]  = NORM * (leftTimeDomain[i] + leftOverlap[i]) + firLeft[i];
        right[i] = NORM * (rightTimeDomain[i] + rightOverlap[i]) + firRight[i];
    }
    std::copy_n(leftTimeDomain.begin() + PARTITION_SIZE, PARTITION_SIZE, leftOverlap.begin());
    std::copy_n(rightTimeDomain.begin() + PARTITION_SIZE, PARTITION_SIZE, rightOverlap.begin());
    historyIndex = (historyIndex + 1) % numberPartitions;
}