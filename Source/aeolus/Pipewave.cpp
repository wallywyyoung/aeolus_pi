//
// Created by Wally Young on 6/14/25.
//

#include "Pipewave.h"
#include <random>
#include <cstring>
#include <memory>
#include <limits>
#include <vector>

AEOLUS_NAMESPACE_BEGIN


Pipewave::Pipewave(std::shared_ptr<Addsynth> model, int note, float freq)
        : _model(model)
        , _note(note)
        , _freq(freq)
        , _needsToBeRebuilt(std::make_shared<std::atomic<bool>>(true))
{
}

Pipewave::Pipewave(const Pipewave& other)
        : _model{ other._model }
        , _note{ other._note }
        , _freq{ other._freq }
        , _sampleRate{ other._sampleRate }
        , _needsToBeRebuilt(std::make_shared<std::atomic<bool>>(other._needsToBeRebuilt->load()))
        , _attackLength{ other._attackLength }
        , _loopLength{ other._loopLength }
        , _sampleStep{ other._sampleStep }
        , _releaseLength{ other._releaseLength }
        , _releaseMultiplier{ other._releaseMultiplier }
        , _releaseDetune{ other._releaseDetune }
        , _instability{ other._instability }
        , _wavetable{ other._wavetable }
        , _attackStartPtr{}
        , _loopStartPtr{}
        , _loopEndPtr{}
{
    // Adjust pointers to the copied wavetable.
    // TODO: Added const_cast is undefined and this will probably break. FIX
    float* wavetable{ const_cast<float*>(other._wavetable.data()) };

    const auto attackOffset = other._attackStartPtr - wavetable;
    const auto loopStartOffset = other._loopStartPtr - wavetable;
    const auto loopEndOffset = other._loopEndPtr - wavetable;

    _attackStartPtr = _wavetable.data() + attackOffset;
    _loopStartPtr = _wavetable.data() + loopStartOffset;
    _loopEndPtr = _wavetable.data() + loopEndOffset;
}

float Pipewave::getPipeFrequency() const noexcept
{
    return _freq * static_cast<float>(_model->getFn()) / static_cast<float>(_model->getFd());
}

void Pipewave::prepateToPlay(float sampleRate)
{
    if (_wavetable.empty() || _sampleRate != sampleRate || _needsToBeRebuilt->load()) {
        _sampleRate = sampleRate;

        genwave();
    }
}

Pipewave::State Pipewave::trigger()
{
    Pipewave::State state = {};

    if (!_needsToBeRebuilt->load()) {
        state.pipewave = this;
        state.env = Pipewave::Attack;
    }

    return state;
}

void Pipewave::release(Pipewave::State& state)
{
    assert(state.env == Pipewave::Attack || state.env == Pipewave::Release);
    assert(state.pipewave == this);
    state.env = Pipewave::Release;
}

void Pipewave::play(Pipewave::State& state, float* out)
{
    static std::random_device rnd;
    std::mt19937 gen(rnd());
    std::uniform_real_distribution<float> dist(std::numeric_limits<float>::min(), std::numeric_limits<float>::max());


    assert(out != nullptr);
    assert(state.env != Pipewave::Idle);
    assert(_attackStartPtr != nullptr);
    assert(_loopStartPtr != nullptr);
    assert(_loopEndPtr != nullptr);

    if (_needsToBeRebuilt->load()) {
        // Drastic measures - pipe has been retuned while playing.
        // Data pointers may be invalid at this point - terminate the voice immediately.
        memset(out, 0, sizeof(float) * SUB_FRAME_LENGTH);
        state.env = Pipewave::Over;
        state.playPtr = nullptr;
        state.releasePtr = nullptr;
        return;
    }

    float* p = state.playPtr;
    float* r = state.releasePtr;

    if (state.env == Pipewave::Attack) {
        if (p == nullptr) {
            p = _attackStartPtr;
            state.playInterpolation = 0.0f;
            state.playInterpolationSpeed = 0.0f;
        }
    } else if (state.env == Pipewave::Release) {
        if (r == nullptr) {
            r = p;
            p = nullptr;
            state.releaseGain = 1.0f;
            state.releaseInterpolation = state.playInterpolation;
            state.releaseCount = _releaseLength;
        }
    } else {
        assert(false); // Invalid envelope state
    }

    if (r != nullptr) {
        int k = SUB_FRAME_LENGTH;
        float* q = out;
        float g = state.releaseGain;
        int i = state.releaseCount - 1;

        float dg = g / SUB_FRAME_LENGTH;

        if (i > 0)
            dg *= _releaseMultiplier;

        if (r < _loopStartPtr) {

            while (k--) {
                *q++ += g * *r++;
                g -= dg;
            }

        } else {

            float y = state.releaseInterpolation;
            float dy = _releaseDetune;

            while (k--) {
                y += dy;

                if (y > 1.0f) {
                    y -= 1.0f;
                    r += 1;
                } else if (y < 0.0f) {
                    y += 1.0f;
                    r -= 1;
                }

                *q++ += g * (r [0] + y * (r [1] - r [0]));
                g -= dg;
                r += _sampleStep;

                if (r >= _loopEndPtr)
                    r -= _loopLength;
            }

            state.releaseInterpolation = y;
        }

        if (i > 0) {
            state.releaseGain = g;
            state.releaseCount = i;
        } else {
            r = nullptr;
            state.env = Pipewave::Over;
        }
    }

    if (p != nullptr) {
        int k = SUB_FRAME_LENGTH;
        float* q = out;

        if (p < _loopStartPtr) {
            while (k--) {
                *q++ += *p++;
            }
        } else {
            float y = state.playInterpolation;
            state.playInterpolationSpeed += _instability * 0.0005f * (0.05f * _instability * (dist(gen) - 0.5f) - state.playInterpolationSpeed);
            float dy = state.playInterpolationSpeed * static_cast<float>(_sampleStep);

            while (k--) {
                y += dy;

                if (y > 1.0f) {
                    y -= 1.0f;
                    p += 1;
                } else if (y < 0.0f) {
                    y += 1.0f;
                    p -= 1;
                }

                *q++ += p [0] + y * (p [1] - p [0]);
                p += _sampleStep;

                if (p >= _loopEndPtr)
                    p -= _loopLength;
            }

            state.playInterpolation = y;
        }
    }

    if (p == nullptr && r == nullptr)
        state.env = Pipewave::Over;

    state.playPtr = p;
    state.releasePtr = r;
}

void Pipewave::genwave()
{
#if ! TARGET_OS_IPHONE
    thread_local
#endif
    static std::random_device rnd;
    std::mt19937 gen(rnd());
    std::uniform_real_distribution<float> dist(std::numeric_limits<float>::min(), std::numeric_limits<float>::max());


    const float sampleRate_r = 1.0f / _sampleRate;

    float m = _model->getNoteAttack(_note);

    for (int h = 0; h < N_HARM; ++h) {
        const float t = _model->getHarmonicAttack(h, _note);

        if (t > m)
            m = t;
    }

    // Attack length aligned to the processing sub-frames
    _attackLength = (int)(_sampleRate * m + 0.5f);
    _attackLength = (_attackLength + SUB_FRAME_LENGTH - 1) & ~(SUB_FRAME_LENGTH - 1);

    // Target frequency
    float f1 = (_freq + _model->getNoteOffset(_note) + _model->getNoteRandomisation(_note) * (2.0f * (dist(gen)) + 1.0f)) * sampleRate_r;

    // Attack frequency (detuned)
    float f0 = f1 * math::exp2ap(_model->getNoteAttackDetune(_note) / 1200.0f);

    float f = 0.0f;

    for (int h = N_HARM - 1; h >= 0; --h) {
        f = (h + 1) * f1;

        if (f < 0.45f && _model->getHarmonicLevel(h, _note) >= -40.0f)
            break;
    }

    if (f > 0.25f)
        _sampleStep = 3;
    else if (f > 0.125f)
        _sampleStep = 2;
    else
        _sampleStep = 1;

    int nc = 0;

    Pipewave::looplen(f1 * _sampleRate, static_cast<float>(_sampleStep) * _sampleRate, (int)(_sampleRate / 6.0f), _loopLength, nc);
    assert(_loopLength > 0);
    assert(nc > 0);

    if (_loopLength < _sampleStep * SUB_FRAME_LENGTH) {
        int k = (_sampleStep * SUB_FRAME_LENGTH - 1) / _loopLength + 1;
        _loopLength *= k;
        nc *= k;
    }

    int wavetableLength = _attackLength + _loopLength + _sampleStep * (SUB_FRAME_LENGTH + 4);
    _wavetable.resize(wavetableLength);

    std::vector<float> arg(wavetableLength);
    std::vector<float> att(wavetableLength);

    _attackStartPtr = _wavetable.data();
    _loopStartPtr = _attackStartPtr + _attackLength;
    _loopEndPtr = _loopStartPtr + _loopLength;

    memset(_attackStartPtr, 0, sizeof(float) * _wavetable.size());

    _releaseLength = (int)(ceilf(_model->getNoteRelease(_note) * _sampleRate / SUB_FRAME_LENGTH) + 1);
    _releaseMultiplier = 1.0f - powf(0.1f, 1.0f / static_cast<float>(_releaseLength));
    _releaseDetune = static_cast<float>(_sampleStep) * (math::exp2ap(_model->getNoteReleaseDetune(_note) / 1200.0f) - 1.0f);
    _instability = _model->getNoteInstability(_note);

    int k = (int)(_sampleRate * _model->getNoteAttack(_note) + 0.5);

    // arg[i] will contain phase steps along the generated wavetable

    {
        float t = 0.0f;

        // Interpolate from frequency f1 to f0 during the attack
        for (int i = 0; i <= _attackLength; ++i) {
            arg [i] = t - floorf(t + 0.5f);
            t += (i < k) ? ((static_cast<float>(k - i) * f0 + static_cast<float>(i) * f1) / static_cast<float>(k)) : f1;
        }
    }

    // Generate phase steps of the sustained loop
    for (int i = 1; i < _loopLength; ++i) {
        float t = arg[_attackLength] + (float)i * static_cast<float>(nc) / static_cast<float>(_loopLength);
        arg[i + _attackLength] = t - floorf(t + 0.5f);
    }

    float v0 = math::exp2ap(0.1661f * _model->getNoteVolume(_note));

    for (int h = 0; h < N_HARM; ++h) {
        if (static_cast<float>(h + 1) * f1 > 0.45f)
            break;

        float v = _model->getHarmonicLevel(h, _note);

        if (v < -80.0f)
            continue;

        v = v0 * math::exp2ap(0.1661f * (v + _model->getHarmonicRandomisation(h, _note) * (2.0f * dist(gen) - 1.0f)));
        k = (int)(_sampleRate * _model->getHarmonicAttack(h, _note) + 0.5f);

        if (k > att.size())
            att.resize(k);

        attgain(att.data(), k, _model->getHarmonicAttackProfile(h, _note));

        for (int i = 0; i < _attackLength + _loopLength; ++i) {
            float t = arg[i] * static_cast<float>(h + 1);
            t -= floorf(t);
            m = v * sinf(M_PI * 2.0f * t);

            if (i < k) {
                m *= att[i];
            }

            _attackStartPtr[i] += m;
        }
    }

    for (int i = 0; i < _sampleStep * (SUB_FRAME_LENGTH + 4); ++i) {
        _attackStartPtr[i + _attackLength + _loopLength] = _attackStartPtr[i + _attackLength];
    }

    _needsToBeRebuilt->store(false);
}

void Pipewave::looplen(float f, float sampleRate, int lmax, int& aa, int& bb)
{
    constexpr int N = 8;
    int z[N];
    int a, b;
    float d;

    float g = sampleRate / f;

    for (int i = 0; i < N; ++i) {
        a = z[i] = (int)(floor (g + 0.5));
        g -= a;
        b = 1;
        int j = i;

        while (j > 0) {
            int t = a;
            a = z[--j] * a + b;
            b = t;
        }

        if (a < 0) {
            a = -a;
            b = -b;
        }

        if (a <= lmax) {
            d = sampleRate * b / a - f;

            if (fabs(d) < 0.1f && fabs(d) < 3e-4f * f)
                break;

            g = (fabs(g) < 1e-6f) ? 1e6f : 1.0f / g;
        } else  {
            b = (int)(static_cast<float>(lmax) * f / sampleRate);
            a = (int)(b * sampleRate / f + 0.5f);
            d = sampleRate * b / a - f;
            break;
        }
    }

    // Avoid zero loops (can happen with some weird tunings).
    aa = std::max(1, a);
    bb = std::max(1, b);
}

void Pipewave::attgain(float* att, int n, float p)
{
    float w = 0.05f;
    float y = 0.6f;

    if (p > 0.0f)
        y += 0.11f * p;

    float z = 0.0;
    int j = 0;

    for (int i = 1; i <= 24; i++)
    {
        int k = n * i / 24;
        float x =  1.0f - z - 1.5f * y;
        y += w * x;
        float d = k == j ? 0.0f : w * y * p / static_cast<float>(k - j);

        while (j < k) {
            float m = (float) j / n;
            att[j++] = (1.0f - m) * z + m;
            z += d;
        }
    }
}

AEOLUS_NAMESPACE_END