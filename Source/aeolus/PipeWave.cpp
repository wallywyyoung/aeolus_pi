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
#include <memory>
#include <random>
#include <vector>

#include "MemoryConstants.h"

PipeWave::PipeWave(const std::shared_ptr<Addsynth> &model, const int note, const float freq) : _model(model), _note(note), _freq(freq) { }

float PipeWave::getPipeFrequency() const noexcept {
    return _freq * static_cast<float>(_model->getFn()) / static_cast<float>(_model->getFd());
}

void PipeWave::play(State &state, std::array<float, PROCESS_FRAMES_SIZE> &out) {
    static std::random_device rnd;
    static std::mt19937 gen(rnd());
    static std::uniform_real_distribution dist(-0.5f, 0.5f);

    float* playbackPosition = state.playbackPosition;
    float* releasePosition = state.releasePosition;

    if (state.envelopeState == Attack) {
        if (playbackPosition == nullptr) {
            playbackPosition = attackWaveformStart;
            state.playInterpolationPhase = 0.0f;
            state.playInterpolationSpeed = 0.0f;
        }
    } else if (state.envelopeState == Release) {
        if (releasePosition == nullptr) {
            releasePosition = playbackPosition;
            playbackPosition = nullptr;
            state.releaseGain = 1.0f;
            state.releaseInterpolationPhase = state.playInterpolationPhase;
            state.remainingReleaseFrames = releaseSampleCount;
        }
    } else {
        assert(false); // Invalid envelope state
    }

    if (releasePosition) {
        int period = PROCESS_FRAMES_SIZE;
        auto playHead = out.begin();
        float releaseGain = state.releaseGain;
        const int remainingReleaseFrames = state.remainingReleaseFrames - 1;

        float gainDecayPerSample = releaseGain / static_cast<float>(PROCESS_FRAMES_SIZE);

        if (remainingReleaseFrames > 0) {
            gainDecayPerSample *= releaseDecayRate;
        }

        if (releasePosition < loopWaveformStart) {
            while (period--) {
                *playHead += releaseGain * *releasePosition;
                ++releasePosition;
                ++playHead;
                releaseGain -= gainDecayPerSample;
            }
        } else {
            float releaseInterpolation = state.releaseInterpolationPhase;
            const auto releaseDetune = _releaseDetune;
            while (period--) {
                releaseInterpolation += releaseDetune;
                if (releaseInterpolation > 1.0f) {
                    --releaseInterpolation;
                    ++releasePosition;
                } else if (releaseInterpolation < 0.0f) {
                    ++releaseInterpolation;
                    --releasePosition;
                }
                *playHead += releaseGain * (releasePosition[0] + releaseInterpolation * (releasePosition[1] - releasePosition[0]));
                ++playHead;
                releaseGain -= gainDecayPerSample;
                releasePosition += _sampleStep;
                if (releasePosition >= _loopEndPtr) {
                    releasePosition -= _loopLength;
                }
            }
            state.releaseInterpolationPhase = releaseInterpolation;
        }
        if (remainingReleaseFrames > 0) {
            state.releaseGain = releaseGain;
            state.remainingReleaseFrames = remainingReleaseFrames;
        } else {
            releasePosition = nullptr;
            state.envelopeState = Over;
        }
    }

    if (playbackPosition) {
        int period = PROCESS_FRAMES_SIZE;
        auto playHead = out.begin();

        if (playbackPosition < loopWaveformStart) {
            while (period--) {
                *playHead += *playbackPosition;
                ++playHead;
                ++playbackPosition;
            }
        } else {
            float playInterpolation = state.playInterpolationPhase;
            state.playInterpolationSpeed += _instability * 0.0005f * (0.05f * _instability * dist(gen) - state.playInterpolationSpeed);
            const float dy = state.playInterpolationSpeed * static_cast<float>(_sampleStep);

            while (period--) {
                playInterpolation += dy;
                if (playInterpolation > 1.0f) {
                    --playInterpolation;
                    ++playbackPosition;
                } else if (playInterpolation < 0.0f) {
                    ++playInterpolation;
                    --playbackPosition;
                }
                // TODO: THIS IS WHERE THIS IS BROKEN
                *playHead += playbackPosition[0] + playInterpolation * (playbackPosition[1] - playbackPosition[0]);
                ++playHead;
                playbackPosition += _sampleStep;
                if (playbackPosition >= _loopEndPtr) {
                    playbackPosition -= _loopLength;
                }
            }
            state.playInterpolationPhase = playInterpolation;
        }
    }
    if (playbackPosition == nullptr && releasePosition == nullptr) {
        state.envelopeState = Over;
    }
    state.playbackPosition = playbackPosition;
    state.releasePosition = releasePosition;
}

void PipeWave::generateWavetable() {
    if (!_wavetable.empty()) {
        return;
    }

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
    _attackLength = static_cast<int>(std::lround(SAMPLE_RATE_F * noteAttack + 0.5f));
    static_assert(isPowerOfTwo(PROCESS_FRAMES_SIZE));
    _attackLength = (_attackLength + PROCESS_FRAMES_SIZE - 1) & ~(PROCESS_FRAMES_SIZE - 1);

    // Target frequency
    const float targetFrequency = (_freq + _model->getNoteOffset(_note) + _model->getNoteRandomisation(_note) * dist(gen)) * SAMPLE_RATE_R;

    // Attack frequency (detuned)
    const float attackFrequency = targetFrequency * math::exp2ap(_model->getNoteAttackDetune(_note) / CENTS_IN_OCTAVE);

    float f = 0.0f;
    for (auto harmonic = HN_func::N_HARM - 1; harmonic >= 0; --harmonic) {
        f = static_cast<float>(harmonic + 1) * targetFrequency;
        if (f < NYQUIST_WITH_MARGIN && _model->getHarmonicLevel(harmonic, _note) >= AUDIBLE_THRESHOLD) {
            break;
        }
    }

    // Oversample higher frequences to avoid aliasing.
    if (f > 0.25f) {
        _sampleStep = 3;
    } else if (f > 0.125f) {
        _sampleStep = 2;
    } else {
        _sampleStep = 1;
    }

    int numberCyclesOfFundamental = 0;
    const float frequencyHz = targetFrequency * SAMPLE_RATE_F;
    const float effectiveSampleRate = SAMPLE_RATE_F / static_cast<float>(_sampleStep);
    looplen(frequencyHz, effectiveSampleRate, static_cast<int>(SAMPLE_RATE_F / 6.0f), _loopLength, numberCyclesOfFundamental);
    assert(_loopLength > 0);
    assert(numberCyclesOfFundamental > 0);

    if (_loopLength < _sampleStep * PROCESS_FRAMES_SIZE) {
        const int k = (_sampleStep * PROCESS_FRAMES_SIZE - 1) / _loopLength + 1;
        _loopLength *= k;
        numberCyclesOfFundamental *= k;
    }

    const int wavetableLength = _attackLength + _loopLength + _sampleStep * (PROCESS_FRAMES_SIZE + 4);
    _wavetable.resize(wavetableLength);
    std::vector<float> arg(wavetableLength);
    std::vector<float> att{};

    attackWaveformStart = _wavetable.data();
    loopWaveformStart = attackWaveformStart + _attackLength;
    _loopEndPtr = loopWaveformStart + _loopLength;
    memset(attackWaveformStart, 0, sizeof(float) * _wavetable.size());

    releaseSampleCount = static_cast<int>(ceilf(_model->getNoteRelease(_note) * SAMPLE_RATE_F / PROCESS_FRAMES_SIZE) + 1);
    releaseDecayRate = 1.0f - powf(0.1f, 1.0f / static_cast<float>(releaseSampleCount));
    _releaseDetune = static_cast<float>(_sampleStep) * (math::exp2ap(_model->getNoteReleaseDetune(_note) / CENTS_IN_OCTAVE) - 1.0f);
    _instability = _model->getNoteInstability(_note);

    const auto attackSampleCount = static_cast<int>(SAMPLE_RATE_F * _model->getNoteAttack(_note) + 0.5);

    // arg[i] will contain phase steps along the generated wavetable

    {
        float t = 0.0f;

        // Interpolate from frequency f1 to f0 during the attack
        for (int i = 0; i <= _attackLength; ++i) {
            arg [i] = t - floorf(t + 0.5f);
            t += (i < attackSampleCount) ? ((static_cast<float>(attackSampleCount - i) * attackFrequency + static_cast<float>(i) * targetFrequency) / static_cast<float>(attackSampleCount)) : targetFrequency;
        }
    }

    // Generate phase steps of the sustained loop
    for (int i = 1; i < _loopLength; ++i) {
        const float t = arg[_attackLength] + static_cast<float>(i) * static_cast<float>(numberCyclesOfFundamental) / static_cast<float>(_loopLength);
        arg[i + _attackLength] = t - floorf(t + 0.5f);
    }

    const float baseNoteAmplitude = math::exp2ap(0.1661f * _model->getNoteVolume(_note));

    for (auto harmonic = 0; harmonic < HN_func::N_HARM; ++harmonic) {
        if (static_cast<float>(harmonic + 1) * targetFrequency > NYQUIST_WITH_MARGIN) {
            break;
        }

        float harmonicLevel = _model->getHarmonicLevel(harmonic, _note);
        if (harmonicLevel < HARMONIC_SKIP_THRESHOLD) {
            continue;
        }
        harmonicLevel = baseNoteAmplitude * math::exp2ap(0.1661f * (harmonicLevel + _model->getHarmonicRandomisation(harmonic, _note) * dist(gen)));

        const auto harmonicAttackSampleCount = static_cast<int>(SAMPLE_RATE_F * _model->getHarmonicAttack(harmonic, _note) + 0.5f);
        if (harmonicAttackSampleCount > att.size()) {
            att.resize(harmonicAttackSampleCount);
        }

        attgain(att.data(), harmonicAttackSampleCount, _model->getHarmonicAttackProfile(harmonic, _note));

        for (int i = 0; i < _attackLength + _loopLength; ++i) {
            float t = arg[i] * static_cast<float>(harmonic + 1);
            t -= floorf(t);
            auto harmonicSample = harmonicLevel * sinf(std::numbers::pi_v<float> * 2.0f * t);

            if (i < harmonicAttackSampleCount) {
                harmonicSample *= att[i];
            }

            attackWaveformStart[i] += harmonicSample;
        }
    }

    for (int i = 0; i < _sampleStep * (PROCESS_FRAMES_SIZE + 4); ++i) {
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
    int a, b;

    auto g = effectiveSampleRate / fundamentalFreqHz;

    for (int i = 0; i < N; ++i) {
        a = z[i] = static_cast<int>(floor(g + 0.5));
        g -= a;
        b = 1;
        int j = i;

        while (j > 0) {
            const int t = a;
            a = z[--j] * a + b;
            b = t;
        }

        if (a < 0) {
            a = -a;
            b = -b;
        }

        if (a <= maxLoopLength) {
            auto d = effectiveSampleRate * b / a - fundamentalFreqHz;

            if (fabs(d) < 0.1f && fabs(d) < 3e-4f * fundamentalFreqHz) {
                break;
            }

            g = (fabs(g) < 1e-6f) ? 1e6f : 1.0f / g;
        } else  {
            b = static_cast<int>(static_cast<float>(maxLoopLength) * fundamentalFreqHz / effectiveSampleRate);
            a = static_cast<int>(b * effectiveSampleRate / fundamentalFreqHz + 0.5f);
            break;
        }
    }

    // Avoid zero loops (can happen with some weird tunings).
    optimalLoopLength = std::max(1, a);
    cycleCount = std::max(1, b);
}

void PipeWave::attgain(float* att, const int n, const float p)
{
    constexpr int N = 24;
    float y = 0.6f;

    if (p > 0.0f) {
        y += 0.11f * p;
    }

    float z = 0.0;
    int j = 0;

    for (auto i = 1; i <= N; i++) {
        constexpr float w = 0.05f;
        const int k = n * i / N;
        const float x =  1.0f - z - 1.5f * y;
        y += w * x;
        const float d = k == j ? 0.0f : w * y * p / static_cast<float>(k - j);

        while (j < k) {
            const float m = static_cast<float>(j) / n;
            att[j++] = (1.0f - m) * z + m;
            z += d;
        }
    }
}
