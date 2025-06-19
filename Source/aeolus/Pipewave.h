//
// Created by Wally Young on 6/14/25.
//
#pragma once

#include "globals.h"
#include "addsynth.h"
#include <atomic>

AEOLUS_NAMESPACE_BEGIN

/**
 * @brief Single pipe wavetable.
 *
 * This class represents a single pipe mapped to a model (additive synth),
 * note, and frequency.
 */
class Pipewave final
{
public:

    /// Envelope state.
    enum EnvState
    {
        Idle,
        Attack,
        Release,
        Over
    };

    /// Playback state.
    struct State
    {
        Pipewave *pipewave = nullptr;
        EnvState env = Idle;
        float* playPtr = nullptr;               // _p_p
        float playInterpolation = 0.0;          // _y_p
        float playInterpolationSpeed = 0.0f;    // _z_p
        float* releasePtr = nullptr;            // _p_r
        float releaseInterpolation = 0.0f;      // _y_r
        float releaseGain = 0.0f;               // _g_r
        int releaseCount = 0;                   // _i_r

        float gain = 1.0f;
        float chiffGain = 0.0f;

        void release() { if (pipewave != nullptr) pipewave->release(*this); }
        [[nodiscard]] bool isTriggered() const noexcept { return pipewave != nullptr && env == Attack; }
        [[nodiscard]] bool isIdle() const noexcept { return env == Idle; }
        [[nodiscard]] bool isOver() const noexcept { return env == Over; }
        void reset() { pipewave = nullptr; env = Idle;}
    };

    Pipewave() = delete;
    Pipewave(std::shared_ptr<Addsynth> model, int note, float freq);
    Pipewave(const Pipewave& other);
    ~Pipewave() = default;


    [[nodiscard]] std::shared_ptr<Addsynth> getModel() const noexcept { return _model; }

    // After changing the frequency of the pipe, the wavetable must be regenerated
    // by calling prepareToPlay() method.
    void setFrequency(float f) noexcept { _freq = f; }
    void setNeedsToBeRebuilt(bool v) noexcept { _needsToBeRebuilt->store(v); }
    [[nodiscard]] bool doesNeedToBeRebuilt() const noexcept { return _needsToBeRebuilt->load(); }

    [[nodiscard]] int getNote() const noexcept { return _note + _model->getNoteMin(); }
    [[nodiscard]] float getFreqency() const noexcept { return _freq; }
    [[nodiscard]] float getPipeFrequency() const noexcept;

    void prepateToPlay(float sampleRate);

    State trigger();
    void release(Pipewave::State& state);

    void play(State& state, float* out);

private:
    void genwave();

    static void looplen(float f, float sampleRate, int lmax, int& aa, int& bb);
    static void attgain(float* att, int n, float p);

    std::shared_ptr<Addsynth> _model;
    int _note;
    float _freq;
    float _sampleRate;

    // Tells whether this pipewave needs to be re-generated.
    // This is required for example when changing the tuninig.
    std::shared_ptr<std::atomic<bool>> _needsToBeRebuilt;

    int _attackLength;          // _l0
    int _loopLength;            // _l1
    int _sampleStep;            // _k_s
    int _releaseLength;         // _k_r
    float _releaseMultiplier;   // _m_r
    float _releaseDetune;       // _d_r
    float _instability;         // _d_p

    std::vector<float> _wavetable;

    float* _attackStartPtr; // _p0
    float* _loopStartPtr;   // _p1
    float* _loopEndPtr;     // _p2
};

AEOLUS_NAMESPACE_END