//
// Created by Wally Young on 11/15/25.
//

#include "aeolus/PipeState.h"
#include <random>

bool PipeState::operator==(const PipeState& other) const {
    return loopStart == other.loopStart;
}

void PipeState::init(const StaticPipe* staticPipe, const float& newOutputGain, const float& newChiffGain) {
    loopEnd = staticPipe->loopEnd;
    loopStart = staticPipe->loopStart;
    position = staticPipe->attackStart;
    interpolationPhase = 0.0f;
    interpolationSpeed = 0.0f;
    gain = 1.0f;
    chiffGain = newChiffGain;
    instability = staticPipe->instability;
    releaseDecayRate = staticPipe->releaseDecayRate;
    releaseDetune = staticPipe->releaseDetune;
    assert(staticPipe->loopLength <= std::numeric_limits<uint16_t>::max());
    loopLength = staticPipe->loopLength;
    assert(staticPipe->releaseFrameCount <= std::numeric_limits<uint16_t>::max());
    remainingReleaseFrames = staticPipe->releaseFrameCount;
    assert(staticPipe->sampleStep <= std::numeric_limits<uint8_t>::max());
    sampleStep = staticPipe->sampleStep;
    assert(staticPipe->getNote() <= std::numeric_limits<uint8_t>::max());
    note = staticPipe->getNote();
    envelopeState = ATTACK;
    outputGain = newOutputGain;
}

int PipeState::getNote() const {
    return note;
}

void PipeState::playMono(std::array<float, PROCESS_FRAMES_SIZE> &out) {
    if (envelopeState == OVER) {
        return;
    }
    auto gainDecay = 0.0f;
    if (envelopeState == RELEASE) {
        static constexpr auto PROCESS_FRAMES_SIZE_R = 1.0f / static_cast<float>(PROCESS_FRAMES_SIZE);
        gainDecay = gain * PROCESS_FRAMES_SIZE_R;
        if (remainingReleaseFrames > 0) {
            gainDecay *= releaseDecayRate;
            --remainingReleaseFrames;
        } else {
            envelopeState = OVER;
        }
    }
    if (position < loopStart) {
        for (auto& sample : out) {
            sample = gain * *position;
            ++position;
            gain -= gainDecay;
        }
    } else {
        auto phaseStep = 0.0f;
        if (envelopeState == ATTACK) {
            static std::random_device rnd;
            static std::mt19937 gen(rnd());
            static std::uniform_real_distribution dist(-0.5f, 0.5f);
            interpolationSpeed += instability * PLAY_INTERPOLATION_SPEED_SCALING * (NOISE_SCALING * instability * dist(gen) - interpolationSpeed);
            phaseStep = interpolationSpeed * static_cast<float>(sampleStep);
        } else { // RELEASE
            phaseStep = releaseDetune;
        }
        for (auto& sample : out) {
            interpolationPhase += phaseStep;

            const int phaseOverflow = (interpolationPhase > 1.0f) - (interpolationPhase < 0.0f);
            position += phaseOverflow;
            interpolationPhase -= static_cast<float>(phaseOverflow);

            position += sampleStep;
            if (position >= loopEnd) {
                position -= loopLength;
            }
            sample = gain * (position [0] + interpolationPhase * (position[1] - position[0]));
            gain -= gainDecay;
        }
    }
}

static_assert(sizeof(PipeState) <= 64, "PipeState must be exactly 64 bytes");
