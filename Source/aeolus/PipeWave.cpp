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

#include "aeolus/PipeWave.h"

#include <cmath>
#include <cstring>
#include <iostream>
#include <memory>
#include <random>
#include <vector>

#include "MemoryConstants.h"

PipeWave::PipeWave(const std::shared_ptr<Addsynth> &model, const int note, const float freq) : _model(model), _note(note), _freq(freq) { }

float PipeWave::getPipeFrequency() const noexcept {
    return _freq * static_cast<float>(_model->getFn()) / static_cast<float>(_model->getFd());
}

void PipeWave::playMono(State &state, std::array<float, PROCESS_FRAMES_SIZE> &out) {
    if (state.envelopeState == Inactive) {
        return;
    }

    assert(state.envelopeState != PipeWave::Idle);
    assert(attackWaveformStart != nullptr);
    assert(loopWaveformStart != nullptr);
    assert(_loopEndPtr != nullptr);

    auto gainDecay = 0.0f;
    if (state.envelopeState == Release) {
        const int remainingReleaseFrames = state.remainingReleaseFrames - 1;
        gainDecay = state.gain / static_cast<float>(PROCESS_FRAMES_SIZE);
        if (remainingReleaseFrames > 0) {
            gainDecay *= releaseDecayRate;
        }
    }
    if (state.position < loopWaveformStart) {
        for (auto& sample : out) {
            sample = state.gain * *state.position;
            ++state.position;
            state.gain -= gainDecay;
        }
    } else {
        const auto phaseStep = (state.*state.getPhaseStep)();
        for (auto& sample : out) {
            state.interpolationPhase += phaseStep;

            const int phaseOverflow = (state.interpolationPhase > 1.0f) - (state.interpolationPhase < 0.0f);
            state.position += phaseOverflow;
            state.interpolationPhase -= static_cast<float>(phaseOverflow);

            state.position += _sampleStep;
            if (state.position >= _loopEndPtr) {
                state.position -= _loopLength;
            }
            sample = state.gain * (state.position [0] + state.interpolationPhase * (state.position[1] - state.position[0]));
            state.gain -= gainDecay;
        }
    }
    if (state.envelopeState == Release) {
        --state.remainingReleaseFrames;
        if (state.remainingReleaseFrames == 0) {
            state.envelopeState = Inactive;
        }
    }
}

void PipeWave::generateWavetable() {
    thread_local std::random_device rnd;
    thread_local std::mt19937 gen(rnd());
    thread_local std::uniform_real_distribution dist(-1.0f, 1.0f);

    float noteAttack = _model->getNoteAttack(_note);

    for (auto harmonic = 0; harmonic < HN_func::N_HARM; ++harmonic) {
        if (const auto harmonicAttack = _model->getHarmonicAttack(harmonic, _note); harmonicAttack > noteAttack) {
            noteAttack = harmonicAttack;
        }
    }

    // Attack length aligned to the processing subframes
    _attackLength = static_cast<int>(std::lround(SAMPLE_RATE_F * noteAttack));
    static_assert(isPowerOfTwo(PROCESS_FRAMES_SIZE));
    _attackLength = (_attackLength + PROCESS_FRAMES_SIZE - 1) & ~(PROCESS_FRAMES_SIZE - 1);

    // Target frequency in Hz - keep in Hz throughout most calculations
    const float targetFrequencyHz = _freq + _model->getNoteOffset(_note) + _model->getNoteRandomisation(_note) * dist(gen);

    // Convert to normalized frequency (cycles per sample) only when needed for phase calculations
    const float targetFrequency = targetFrequencyHz * SAMPLE_RATE_R;

    // Attack frequency (detuned) in Hz
    const float attackFrequencyHz = targetFrequencyHz * math::exp2ap(_model->getNoteAttackDetune(_note) / CENTS_IN_OCTAVE);
    const float attackFrequency = attackFrequencyHz * SAMPLE_RATE_R;

    // Find the highest significant harmonic frequency in Hz to determine anti-aliasing
    float highestHarmonicFreqHz = 0.0f;
    for (auto harmonic = 0; harmonic < HN_func::N_HARM; ++harmonic) {
        const float harmonicFreqHz = static_cast<float>(harmonic + 1) * targetFrequencyHz;
        if (harmonicFreqHz > SAMPLE_RATE_F * 0.45f) { // Stop before Nyquist with margin
            break;
        }
        if (_model->getHarmonicLevel(harmonic, _note) >= AUDIBLE_THRESHOLD) {
            highestHarmonicFreqHz = harmonicFreqHz;
        }
    }

    // Improved anti-aliasing: oversample based on highest harmonic frequency
    if (highestHarmonicFreqHz > SAMPLE_RATE_F * 0.35f) {
        _sampleStep = 4;  // Heavy oversampling for very high frequencies
    } else if (highestHarmonicFreqHz > SAMPLE_RATE_F * 0.25f) {
        _sampleStep = 3;
    } else if (highestHarmonicFreqHz > SAMPLE_RATE_F * 0.15f) {
        _sampleStep = 2;
    } else {
        _sampleStep = 1;
    }

    auto numberCyclesOfFundamental = 0;

    // Pass frequency in Hz to looplen function
    looplen(targetFrequencyHz, SAMPLE_RATE_F / static_cast<float>(_sampleStep), static_cast<int>(SAMPLE_RATE_F / 6.0f), _loopLength, numberCyclesOfFundamental);
    assert(_loopLength > 0);
    assert(numberCyclesOfFundamental > 0);

    if (_loopLength < _sampleStep * PROCESS_FRAMES_SIZE) {
        const int k = (_sampleStep * PROCESS_FRAMES_SIZE - 1) / _loopLength + 1;
        _loopLength *= k;
        numberCyclesOfFundamental *= k;
    }

    const int wavetableLength = _attackLength + _loopLength + _sampleStep * (PROCESS_FRAMES_SIZE + 4);
    _wavetable.resize(wavetableLength);

    std::vector<float> phaseSteps(wavetableLength);
    std::vector<float> att{};

    attackWaveformStart = _wavetable.data();
    loopWaveformStart = attackWaveformStart + _attackLength;
    _loopEndPtr = loopWaveformStart + _loopLength;

    memset(attackWaveformStart, 0, sizeof(float) * _wavetable.size());

    releaseFrameCount = static_cast<int>(ceilf(_model->getNoteRelease(_note) * SAMPLE_RATE_F / PROCESS_FRAMES_SIZE) + 1);
    releaseDecayRate = 1.0f - powf(0.1f, 1.0f / static_cast<float>(releaseFrameCount));
    _releaseDetune = static_cast<float>(_sampleStep) * (math::exp2ap(_model->getNoteReleaseDetune(_note) / CENTS_IN_OCTAVE) - 1.0f);
    _instability = _model->getNoteInstability(_note);

    // Use the maximum attack time for all harmonics
    const auto attackSampleCount = static_cast<int>(std::lround(SAMPLE_RATE_F * noteAttack));

    // phaseSteps[i] will contain phase steps along the generated wavetable

    {
        auto t = 0.0f;
        // Interpolate from attack frequency to target frequency during the attack
        for (auto i = 0; i <= _attackLength; ++i) {
            phaseSteps[i] = t - floorf(t + 0.5f);
            t += (i < attackSampleCount) ? ((static_cast<float>(attackSampleCount - i) * attackFrequency + static_cast<float>(i) * targetFrequency) / static_cast<float>(attackSampleCount)) : targetFrequency;
        }
    }

    // Generate phase steps of the sustained loop - improved precision
    const float phaseIncrement = static_cast<float>(numberCyclesOfFundamental) / static_cast<float>(_loopLength);
    for (auto i = 1; i < _loopLength; ++i) {
        const float t = phaseSteps[_attackLength] + static_cast<float>(i) * phaseIncrement;
        phaseSteps[i + _attackLength] = t - floorf(t + 0.5f);
    }

    const float baseNoteAmplitude = math::exp2ap(DECIBEL_TO_LINEAR_APPROX * _model->getNoteVolume(_note));

    for (auto harmonic = 0; harmonic < HN_func::N_HARM; ++harmonic) {

        // Strict anti-aliasing: skip harmonics that would alias
        if (const float harmonicFreqHz = static_cast<float>(harmonic + 1) * targetFrequencyHz;
            harmonicFreqHz > SAMPLE_RATE_F * 0.45f) {
            break;
        }

        float harmonicLevel = _model->getHarmonicLevel(harmonic, _note);

        if (harmonicLevel < HARMONIC_SKIP_THRESHOLD) {
            continue;
        }
        harmonicLevel = baseNoteAmplitude * math::exp2ap(DECIBEL_TO_LINEAR_APPROX * (harmonicLevel + _model->getHarmonicRandomisation(harmonic, _note) * dist(gen)));

        // Use unified attack sample count for all harmonics
        const auto harmonicAttackSampleCount = attackSampleCount;
        if (harmonicAttackSampleCount > att.size()) {
            att.resize(harmonicAttackSampleCount);
        }

        attgain(att.data(), harmonicAttackSampleCount, _model->getHarmonicAttackProfile(harmonic, _note));

        // Generate harmonic with improved phase precision
        for (auto i = 0; i < _attackLength + _loopLength; ++i) {
            // Use double precision for phase calculation to avoid accumulation errors
            double t = static_cast<double>(phaseSteps[i]) * static_cast<double>(harmonic + 1);
            t -= std::floor(t);
            auto harmonicSample = harmonicLevel * std::sinf(std::numbers::pi_v<float> * 2.0f * static_cast<float>(t));

            if (i < harmonicAttackSampleCount) {
                harmonicSample *= att[i];
            }

            attackWaveformStart[i] += harmonicSample;
        }
    }

    for (auto i = 0; i < _sampleStep * (PROCESS_FRAMES_SIZE + 4); ++i) {
        attackWaveformStart[i + _attackLength + _loopLength] = attackWaveformStart[i + _attackLength];
    }
}

/**
 * @brief Find a loop length _loopLength and cycle count nc
 * Satisfies the constraints:
 * 1) the loop contains exactly nc cycles of the fundamental frequency.
 * 2) effectiveSampleRate * nc / _loopLength ≈ freqHz
 */
void PipeWave::looplen(const float fundamentalFreqHz, const float effectiveSampleRate, const int maxLoopLength, int &optimalLoopLength, int &cycleCount)
{
    constexpr int N = 8;
    int z[N];
    int integerPart, b;
    float d;

    auto fractionalPart = effectiveSampleRate / fundamentalFreqHz;

    // Euclidian algorithm for continue fractions.
    for (auto i = 0; i < N; ++i) {
        integerPart = z[i] = static_cast<int>(floor(fractionalPart));
        fractionalPart -= static_cast<float>(integerPart);
        b = 1;
        int j = i;

        while (j > 0) {
            const auto t = integerPart;
            integerPart = z[--j] * integerPart + b;
            b = t;
        }

        if (integerPart < 0) {
            integerPart = -integerPart;
            b = -b;
        }

        if (integerPart <= maxLoopLength) {
            d = effectiveSampleRate * static_cast<float>(b) / static_cast<float>(integerPart) - fundamentalFreqHz;

            if (fabs(d) < 0.1f && fabs(d) < 3e-4f * fundamentalFreqHz) {
                break;
            }

            fractionalPart = (fabs(fractionalPart) < 1e-6f) ? 1e6f : 1.0f / fractionalPart;
        } else  {
            b = static_cast<int>(static_cast<float>(maxLoopLength) * fundamentalFreqHz / effectiveSampleRate);
            integerPart = static_cast<int>(std::lround(static_cast<float>(b) * effectiveSampleRate / fundamentalFreqHz));
            d = effectiveSampleRate * b / integerPart - fundamentalFreqHz;
            if (std::fabs(d) > 1.0f) {
                std::cerr << "LoopLen: Large error rate. " << d << std::endl;
            }
            break;
        }
    }

    // Avoid zero loops (can happen with some weird tunings).
    optimalLoopLength = std::max(1, integerPart);
    cycleCount = std::max(1, b);
}

void PipeWave::attgain(float *att, const int &n, const float &p) {
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
            const auto m = static_cast<float>(j) / static_cast<float>(n);
            att[j++] = (1.0f - m) * z + m;
            z += d;
        }
    }
}

