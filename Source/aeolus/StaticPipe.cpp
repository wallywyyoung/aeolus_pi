// ----------------------------------------------------------------------------
//
//  Copyright (C) 2025 Wally Young <wallywyyoung@users.noreply.github.com>
//  Copyright (C) 2021 Arthur Benilov <arthur.benilov@gmail.com>
//  Copyright (C) 2003-2013 Fons Adriaensen <fons@linuxaudio.org>
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
// ---------------------------------------------------------------------------

#include "aeolus/StaticPipe.h"
#include <arm_math.h>
#include <iostream>
#include <memory>
#include <numbers>
#include <random>
#include <vector>
#include "MemoryConstants.h"
#include "WavetableMemoryManager.h"

StaticPipe::StaticPipe(const std::shared_ptr<AddSynth> &model, const int note, const float freq) : model(model), note(note), freq(freq) { }

float StaticPipe::getPipeFrequency() const noexcept {
    return freq;
}

void StaticPipe::generateWavetable() {
    thread_local std::random_device rnd;
    thread_local std::mt19937 gen(rnd());
    thread_local std::uniform_real_distribution dist(-1.0f, 1.0f);

    float noteAttack = model->getNoteAttackTime(note);

    for (auto harmonic = 0; harmonic < HN_func::N_HARM; ++harmonic) {
        if (const auto harmonicAttack = model->getHarmonicAttack(harmonic, note); harmonicAttack > noteAttack) {
            noteAttack = harmonicAttack;
        }
    }

    // Attack length aligned to the processing subframes
    attackLength = static_cast<int>(std::lround(SAMPLE_RATE_F * noteAttack));
    attackLength = (attackLength + static_cast<int>(PROCESS_FRAMES_SIZE) - 1) & ~(static_cast<int>(PROCESS_FRAMES_SIZE) - 1);

    // Target frequency in Hz - keep in Hz throughout most calculations
    const float targetFrequencyHz = freq + model->getNoteFrequencyOffset(note) + model->getNoteRandomisation(note) * dist(gen);

    // Convert to normalized frequency (cycles per sample) only when needed for phase calculations
    const float targetFrequency = targetFrequencyHz * SAMPLE_RATE_R;

    // Attack frequency (detuned) in Hz
    const float attackFrequencyHz = targetFrequencyHz * math::exp2ap(model->getNoteAttackDetune(note) / CENTS_IN_OCTAVE);
    const float attackFrequency = attackFrequencyHz * SAMPLE_RATE_R;

    // Find the highest significant harmonic frequency in Hz to determine anti-aliasing
    float highestHarmonicFreqHz = 0.0f;
    for (auto harmonic = 0; harmonic < HN_func::N_HARM; ++harmonic) {
        const float harmonicFreqHz = static_cast<float>(harmonic + 1) * targetFrequencyHz;
        if (harmonicFreqHz > SAMPLE_RATE_F * 0.45f) { // Stop before Nyquist with margin
            break;
        }
        if (model->getHarmonicLevel(harmonic, note) >= AUDIBLE_THRESHOLD) {
            highestHarmonicFreqHz = harmonicFreqHz;
        }
    }

    // Improved anti-aliasing: oversample based on highest harmonic frequency
    if (highestHarmonicFreqHz > SAMPLE_RATE_F * 0.35f) {
        sampleStep = 4;  // Heavy oversampling for very high frequencies
    } else if (highestHarmonicFreqHz > SAMPLE_RATE_F * 0.25f) {
        sampleStep = 3;
    } else if (highestHarmonicFreqHz > SAMPLE_RATE_F * 0.15f) {
        sampleStep = 2;
    } else {
        sampleStep = 1;
    }
    auto numberCyclesOfFundamental = 0;

    // Pass frequency in Hz to calculateLoopLength function
    calculateLoopLength(targetFrequencyHz, SAMPLE_RATE_F / static_cast<float>(sampleStep), static_cast<int>(SAMPLE_RATE_F / 6.0f), loopLength, numberCyclesOfFundamental);
    assert(loopLength > 0);
    assert(numberCyclesOfFundamental > 0);

    if (loopLength < sampleStep * PROCESS_FRAMES_SIZE) {
        const int k = (sampleStep * PROCESS_FRAMES_SIZE - 1) / loopLength + 1;
        loopLength *= k;
        numberCyclesOfFundamental *= k;
    }

    const int wavetableLength = attackLength + loopLength + sampleStep * (PROCESS_FRAMES_SIZE + 4);
    attackStart = WavetableMemoryManager::getInstance().allocateWavetable(wavetableLength);
    std::fill_n(attackStart, wavetableLength, 0.0f);
    loopStart = attackStart + attackLength;
    loopEnd = loopStart + loopLength;

    std::vector<float> phaseSteps(wavetableLength);
    std::vector<float> att{};

    releaseFrameCount = static_cast<int>(ceilf(model->getNoteDecayTime(note) * SAMPLE_RATE_F / PROCESS_FRAMES_SIZE) + 1);
    releaseDecayRate = 1.0f - powf(0.1f, 1.0f / static_cast<float>(releaseFrameCount));
    releaseDetune = static_cast<float>(sampleStep) * (math::exp2ap(model->getNoteDecayDetune(note) / CENTS_IN_OCTAVE) - 1.0f);
    instability = model->getNoteInstability(note);

    // Use the maximum attack time for all harmonics
    const auto attackSampleCount = static_cast<int>(std::lround(SAMPLE_RATE_F * noteAttack));

    // phaseSteps[i] will contain phase steps along the generated wavetable

    {
        auto t = 0.0f;
        // Interpolate from attack frequency to target frequency during the attack
        for (auto i = 0; i <= attackLength; ++i) {
            phaseSteps[i] = std::fmod(t, 1.0f);
            t += (i < attackSampleCount) ? ((static_cast<float>(attackSampleCount - i) * attackFrequency + static_cast<float>(i) * targetFrequency) / static_cast<float>(attackSampleCount)) : targetFrequency;
        }
    }

    // Generate phase steps of the sustained loop - improved precision
    const float phaseIncrement = static_cast<float>(numberCyclesOfFundamental) / static_cast<float>(loopLength);
    for (auto i = 1; i < loopLength; ++i) {
        const float t = phaseSteps[attackLength] + static_cast<float>(i) * phaseIncrement;
        phaseSteps[i + attackLength] = std::fmod(t, 1.0f);
    }

    const float baseNoteAmplitude = math::exp2ap(DECIBEL_TO_LINEAR_APPROX * model->getNoteVolume(note));

    for (auto harmonic = 0; harmonic < HN_func::N_HARM; ++harmonic) {
        // Strict anti-aliasing: skip harmonics that would alias
        if (const float harmonicFreqHz = static_cast<float>(harmonic + 1) * targetFrequencyHz;
            harmonicFreqHz > SAMPLE_RATE_F * 0.45f) {
            break;
        }

        float harmonicLevel = model->getHarmonicLevel(harmonic, note);

        if (harmonicLevel < HARMONIC_SKIP_THRESHOLD) {
            continue;
        }
        harmonicLevel = baseNoteAmplitude * math::exp2ap(DECIBEL_TO_LINEAR_APPROX * (harmonicLevel + model->getHarmonicRandomisation(harmonic, note) * dist(gen)));

        const auto harmonicAttackSampleCount = static_cast<int>(std::round(SAMPLE_RATE_F * model->getHarmonicAttack(harmonic, note)));
        if (harmonicAttackSampleCount > att.size()) {
            att.resize(harmonicAttackSampleCount);
        }

        attgain(att.data(), harmonicAttackSampleCount, model->getHarmonicAttackProfile(harmonic, note));

        // Generate harmonic with improved phase precision
        for (auto i = 0; i < attackLength + loopLength; ++i) {
            // Use double precision for phase calculation to avoid accumulation errors
            double t = static_cast<double>(phaseSteps[i]) * static_cast<double>(harmonic + 1);
            t -= std::floor(t);
            auto harmonicSample = harmonicLevel * std::sinf(std::numbers::pi_v<float> * 2.0f * static_cast<float>(t));

            if (i < harmonicAttackSampleCount) {
                harmonicSample *= att[i];
            }

            attackStart[i] += harmonicSample;
        }
    }

    for (auto i = 0; i < sampleStep * (PROCESS_FRAMES_SIZE + 4); ++i) {
        attackStart[i + attackLength + loopLength] = attackStart[i + attackLength];
    }
}

/**
 * @brief Find a loop length _loopLength and cycle count nc
 * Satisfies the constraints:
 * 1) the loop contains exactly nc cycles of the fundamental frequency.
 * 2) effectiveSampleRate * nc / _loopLength ≈ freqHz
 */
void StaticPipe::calculateLoopLength(const float fundamentalFreqHz, const float effectiveSampleRate, const int maxLoopLength, int &optimalLoopLength, int &cycleCount)
{
    constexpr int N = 8;
    int fractionIntegerParts[N];
    int integerPart;
    double frequencyDeviation;

    auto fractionalPart = static_cast<double>(effectiveSampleRate / fundamentalFreqHz);
    // Euclidian algorithm for continue fractions.
    for (auto i = 0; i < N; ++i) {
        integerPart = fractionIntegerParts[i] = static_cast<int>(std::round(fractionalPart));
        fractionalPart -= static_cast<float>(integerPart);
        cycleCount = 1;
        auto j = i;

        while (j > 0) {
            const auto t = integerPart;
            integerPart = fractionIntegerParts[--j] * integerPart + cycleCount;
            cycleCount = t;
        }

        if (integerPart < 0) {
            integerPart = -integerPart;
            cycleCount = -cycleCount;
        }

        if (integerPart <= maxLoopLength) {
            frequencyDeviation = effectiveSampleRate * static_cast<float>(cycleCount) / static_cast<float>(integerPart) - fundamentalFreqHz;

            if (fabs(frequencyDeviation) < 0.1 && fabs(frequencyDeviation) < 3e-4 * fundamentalFreqHz) {
                break;
            }

            fractionalPart = (fabs(fractionalPart) < 1e-6) ? 1e6 : 1.0 / fractionalPart;
        } else  {
            cycleCount = static_cast<int>(static_cast<float>(maxLoopLength) * fundamentalFreqHz / effectiveSampleRate);
            integerPart = static_cast<int>(std::lround(static_cast<float>(cycleCount) * effectiveSampleRate / fundamentalFreqHz));
            frequencyDeviation = static_cast<double>(effectiveSampleRate) * static_cast<double>(cycleCount) / static_cast<double>(integerPart) - static_cast<double>(fundamentalFreqHz);
            if (std::fabs(frequencyDeviation) > 1.0f) {
                std::cerr << "LoopLen: Large error rate. " << frequencyDeviation << std::endl;
            }
            break;
        }
    }

    // Avoid zero loops (can happen with some weird tunings).
    optimalLoopLength = std::max(1, std::abs(integerPart));
    cycleCount = std::max(1, std::abs(cycleCount));
}

void StaticPipe::attgain(float *att, const int &n, const float &p) {
    auto w = 0.05f;
    auto y = 0.6f;

    if (p > 0.0f) {
        y += 0.11f * p;
    }

    auto z = 0.0f;
    auto j = 0;

    for (auto i = 1; i <= 24; i++)
    {
        const auto k = n * i / 24;
        const auto x =  1.0f - z - 1.5f * y;
        y += w * x;
        const auto d = k == j ? 0.0f : w * y * p / static_cast<float>(k - j);

        while (j < k) {
            const auto m = static_cast<double>(j) / static_cast<double>(n);
            att[j++] = static_cast<float>((1.0 - m) * static_cast<double>(z) + static_cast<double>(m));
            z += d;
        }
    }
}

