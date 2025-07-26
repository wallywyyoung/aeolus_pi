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
// ---------------------------------------------------------------------------

#include "aeolus/globals.h"
#include "aeolus/Division.h"
#include "aeolus/engine.h"
#include "aeolus/EngineGlobal.h"

Division::Division(const Engine& engine, const std::string& name)
    : _name{name}
    , _mnemonic{name}
    , _hasSwell{false}
    , _hasTremulant{false}
    , _midiChannelsMask{ (1 << 16) - 1 }
    , _tremulantEnabled{false} // Select all MIDI channels by default
    , _tremulantLevel{0.0f}
    , _tremulantMaxLevel{TREMULANT_TARGET_LEVEL}
    , _tremulantTargetLevel{0.0f}
    , _params{NUM_PARAMS}
    , _swellFilterSpec{}
    , _swellFilterStateL{}
    , _swellFilterStateR{}
    , _tremulantDelayL(TREMULANT_DELAY_LENGTH)
    , _tremulantDelayR(TREMULANT_DELAY_LENGTH)
    , _triggerFlag{}
    , _engine{engine}
{
    _swellFilterSpec.type = dsp::BiquadFilter::LowPass;
    _swellFilterSpec.sampleRate = SAMPLE_RATE_F;
    _swellFilterSpec.dbGain = 0.0f;
    _swellFilterSpec.q = 0.7071f;
    _swellFilterSpec.freq = 0.4f * SAMPLE_RATE_F;

    dsp::BiquadFilter::updateSpec(_swellFilterSpec);
    dsp::BiquadFilter::resetState(_swellFilterSpec, _swellFilterStateL);
    dsp::BiquadFilter::resetState(_swellFilterSpec, _swellFilterStateR);
}

void Division::clearLinkedDivisions()
{
    _linkedDivisions.clear();
    _linkedFromDivisions.clear();
}

void Division::populateLinkedDivisions()
{
    for (const auto& name : _linkedDivisionNames) {
        if (const auto division = _engine.getDivisionByName(name)) {
            Link link{ division, false };
            _linkedDivisions.push_back(link);
            division->_linkedFromDivisions.push_back(this);
        }
    }
}

int Division::getLinksCount() const noexcept
{
    return static_cast<int>(_linkedDivisions.size());
}

void Division::enableLink(const int i, const bool ena)
{
    isPositiveAndBelow(i, _linkedDivisions.size());

    if (_linkedDivisions[i].enabled != ena) {
        _linkedDivisions[i].enabled = ena;
        _engine.getSequencer().setCurrentStepDirty();
    }
}

bool Division::isLinkEnabled(const int i) const {
    isPositiveAndBelow(i, _linkedDivisions.size());
    return _linkedDivisions[i].enabled;
}

Division::Link& Division::getLinkByIndex(const int i)
{
    isPositiveAndBelow(i, _linkedDivisions.size());
    return _linkedDivisions[i];
}

void Division::cancelAllLinks()
{
    bool changed{ false };

    for (auto&[division, enabled] : _linkedDivisions) {
        if (enabled) {
            changed = true;
            enabled = false;
        }
    }

    if (changed)
        _engine.getSequencer().setCurrentStepDirty();
}

void Division::clear()
{
    _stops.clear();
}

Stop& Division::addRankwave(Rankwave *ptr, const bool ena, const std::string& name) {
    assert(ptr != nullptr);
    auto stop = Stop();
    stop.addZone(ptr);
    stop.setEnabled(ena);
    stop.setName(name.empty() ? ptr->getStopName() : name);
    _stops.push_back(stop);
    return _stops.back();
}

int Division::getStopsCount() const noexcept
{
    return static_cast<int>(_stops.size());
}

void Division::enableStop(const int i, const bool ena)
{
    isPositiveAndBelow(i, _stops.size());

    if (_stops[i].isEnabled() != ena) {
        _stops[i].setEnabled(ena);
        _engine.getSequencer().setCurrentStepDirty();
    }
}

bool Division::isStopEnabled(const int i) const
{
    isPositiveAndBelow(i, _stops.size());
    return _stops[i].isEnabled();
}

Stop& Division::getStopByIndex(const int i)
{
    isPositiveAndBelow(i, _stops.size());
    return _stops[i];
}

void Division::disableAllStops()
{
    for (int i = 0; i < getStopsCount(); ++i)
        enableStop(i, false);
}

void Division::getAvailableRange(int& minNote, int& maxNote) const noexcept
{
    minNote = -1;
    maxNote = -1;

    for (const auto& stop : _stops) {
        if (stop.isEnabled()) {
            const auto range{stop.getKeyRange()};

            if (minNote < 0 || minNote > range.getStart())
                minNote = range.getStart();
            if (maxNote < 0 || maxNote < range.getEnd() - 1)
                maxNote = range.getEnd() - 1;
        }
    }
}

bool Division::isForMIDIChannel(const int channel) const noexcept
{
    const int mask{ _midiChannelsMask.load() };
    return midi::matchMidiChannelToMask(mask, channel);
}

void Division::setTremulantEnabled(const bool ena) noexcept
{
    if (!_hasTremulant)
        return;

    if (_tremulantEnabled != ena) {
        _tremulantEnabled = ena;
        _tremulantTargetLevel = _tremulantEnabled ? _tremulantMaxLevel : 0.0f;

        _engine.getSequencer().setCurrentStepDirty();
    }
}

float Division::getTremulantLevel(const bool update)
{
    const auto level = _tremulantLevel;

    if (update)
        _tremulantLevel += 0.1f * (_tremulantTargetLevel - _tremulantLevel);

    return level;
}

void Division::noteOn(const int note, const int midiChannel)
{
    if (hasBeenTriggered())
        return;

    if (!isForMIDIChannel(midiChannel))
        return;

    _triggerFlag = true;

    for (int stopIndex = 0; stopIndex < static_cast<int>(_stops.size()); ++stopIndex)
        triggerVoicesForStop(stopIndex, note);

    if (midiChannel != 0) {
        // Update keys state only when triggered by the assigned MIDI channel
        _keysState.set(note);
    }

    // Forward to the linked divisions
    for (auto&[division, enabled] : _linkedDivisions) {
        if (enabled) {
            division->noteOn(note, 0);
        }
    }
}

void Division::noteOff(const int note, const int midiChannel)
{
    if (hasBeenTriggered())
        return;

    if (!isForMIDIChannel(midiChannel))
        return;

    _triggerFlag = true;

    auto* voice = _activeVoices.first();

    while (voice != nullptr) {
        if (voice->isForNote(note))
            voice->release();

        voice = voice->next();
    }

    if (midiChannel != 0) {
        // Update keys state only when triggered by the assigned MIDI channel.
        _keysState.reset(note);
    }

    // Forward to the linked divisions
    for (auto&[division, enabled] : _linkedDivisions) {
        if (enabled) {
            division->noteOff(note, 0);
        }
    }
}

void Division::allNotesOff()
{
    _keysState.reset();

    auto* voice = _activeVoices.first();

    while (voice != nullptr) {
        voice->release();
        voice = voice->next();
    }
}

void Division::handleControlMessage(const MidiData& msg)
{
    const int cc{ msg.param };

    if (cc != CC_MODULATION && cc != CC_VOLUME && cc != CC_ALL_NOTES_OFF)
        return;

    const int swellCh{ EngineGlobal::getInstance()->getMIDISwellChannelsMask() };
    const float value{ static_cast<float>(msg.value) / 127.0f };

    if (msg.channel == 0 || (swellCh & (msg.channel - 1)) != 0) {
        if (_hasSwell && cc == CC_VOLUME) {
            _params[GAIN].setValue(value);
        }
    }

    // Hange manual channel specific controls
    if (!isForMIDIChannel(msg.channel))
        return;

    if (cc == CC_MODULATION && hasTremulant())
        setTremulantEnabled(value > 0.5f);

    if (cc == CC_ALL_NOTES_OFF)
        allNotesOff();
}

bool Division::process(StaticAudioBuffer<SUB_FRAME_LENGTH, N_OUTPUT_CHANNELS>& targetBuffer, StaticAudioBuffer<SUB_FRAME_LENGTH, N_OUTPUT_CHANNELS>& voiceBuffer) {
    updateAggregatedKeysState();
    releaseVoicesOfDisabledStops();
    triggerVoicesOfEnabledStops();

    auto* voice = _activeVoices.first();

    if (voice == nullptr)
        return false;

    while (voice != nullptr) {
        voiceBuffer.clear();
        float* outL = voiceBuffer.getWritePointer(0);
        float* outR = voiceBuffer.getNumChannels() > 1 ? voiceBuffer.getWritePointer(1) : outL;

        voice->process(outL, outR);

#if AEOLUS_MULTIBUS_OUTPUT
        // Mix voice to the corresponding output channel depending on the pan-position
        int ch = Limit(0, targetBuffer.getNumChannels() - 1, int(voice->getPanPosition() * targetBuffer.getNumChannels()));
        targetBuffer.addFrom(ch, 0, voiceBuffer, 0, 0, SUB_FRAME_LENGTH);
#else
        targetBuffer.addFrom(0, 0, voiceBuffer, 0, 0, SUB_FRAME_LENGTH);
        targetBuffer.addFrom(1, 0, voiceBuffer, 1, 0, SUB_FRAME_LENGTH);
#endif
        if (voice->isOver()) {
            auto* nextVoice = _activeVoices.removeAndReturnNext(voice);
            voice->resetAndReturnToPool();
            voice = nextVoice;
        } else {
            voice = voice->next();
        }
    }

    return true;
}

void Division::modulate(StaticAudioBuffer<SUB_FRAME_LENGTH, N_OUTPUT_CHANNELS>& targetBuffer, const StaticAudioBuffer<SUB_FRAME_LENGTH, 1>& tremulantBuffer)
{
    const float* gain = tremulantBuffer.getReadPointer(0);
    assert(gain != nullptr);

    const float lvl = getTremulantLevel(true);

    // Update gain smoothly
    auto& paramGain = _params[GAIN];
//    paramGain.setValue(_paramGain->get());

#if AEOLUS_MULTIBUS_OUTPUT

    if (paramGain.isSmoothing()) {

        for (int i = 0; i < SUB_FRAME_LENGTH; ++i) {
            const float g = (1.0f + gain[i] * lvl) * paramGain.nextValue();

            for (int ch = 0; ch < targetBuffer.getNumChannels(); ++ch) {
                targetBuffer.getWritePointer(ch)[i] *= g;
            }
        }

    } else {
        // Gain is stable
        const float pgain = paramGain.target();

        for (int ch = 0; ch < targetBuffer.getNumChannels(); ++ch) {
            float* const out = targetBuffer.getWritePointer(ch);

            for (int i = 0; i < SUB_FRAME_LENGTH; ++i) {
                float g = (1.0f + gain[i] * lvl) * pgain;
                out[i] *= g;
            }
        }
    }

    // No swell filtering for multibus

#else

    float* outL = targetBuffer.getWritePointer(0);
    float* outR = targetBuffer.getWritePointer(1);

    assert(outL != nullptr);
    assert(outR != nullptr);

    for (int i = 0; i < SUB_FRAME_LENGTH; ++i) {
        _tremulantDelayL.write(outL[i]);
        _tremulantDelayR.write(outR[i]);

        const float g = (1.0f + gain[i] * lvl) * paramGain.nextValue();

        constexpr float freqModCenter = TREMULANT_DELAY_LENGTH * 0.5f;
        constexpr float freqModAmp = TREMULANT_DELAY_LENGTH * 0.5f * TREMULANT_DELAY_MODULATION_LEVEL;

        const float p = freqModCenter + freqModAmp * (0.5f - gain[i] * lvl);
        outL[i] = _tremulantDelayL.read(p) * g;
        outR[i] = _tremulantDelayR.read(p) * g;
    }

    // Apply swell filter
    if (hasSwell()) {
        // Close the filter along with the gain
        const float k = powf(limitRange(0.0f, 1.0f, paramGain.target()), 1.3f);
        _swellFilterSpec.freq = 400.0f + k * (18000.0f - 400.0f);
        dsp::BiquadFilter::updateSpec(_swellFilterSpec);
        dsp::BiquadFilter::process(_swellFilterSpec, _swellFilterStateL, outL, outL, SUB_FRAME_LENGTH);
        dsp::BiquadFilter::process(_swellFilterSpec, _swellFilterStateR, outR, outR, SUB_FRAME_LENGTH);
    }
#endif
}

void Division::releaseVoicesOfDisabledStops()
{
    auto* voice = _activeVoices.first();

    while (voice != nullptr) {
        bool shouldRelease{ false };

        if (voice->isActive()) {
            if (const int stopIndex = voice->stopIndex(); isPositiveAndBelow(stopIndex, _stops.size())) {
                if (const auto& stop = _stops[stopIndex]; !stop.isEnabled())
                    shouldRelease = true;

                if (voice->getNote() >= 0 && !_aggregatedKeysState[voice->getNote()])
                    shouldRelease = true;
            }
        }

        if (shouldRelease)
            voice->release();

        voice = voice->next();
    }
}

void Division::triggerVoicesOfEnabledStops()
{
    if (_aggregatedKeysState.none()) {
        // No keys are pressed
        return;
    }

    std::bitset missingNotes{ _aggregatedKeysState };

    for (int stopIndex = 0; stopIndex < _stops.size(); ++stopIndex) {
        if (auto& stop = _stops[stopIndex]; !stop.isEnabled())
            continue;

        bool hasVoices = false;

        auto* voice = _activeVoices.first();

        while (voice != nullptr) {
            if (const int voiceNote{ voice->getNote() }; voice->stopIndex() == stopIndex && voiceNote >= 0 && _aggregatedKeysState[voiceNote]) {
                hasVoices = true;
                missingNotes[voiceNote] = false;
                break;
            }

            voice = voice->next();
        }

        // Trigger voices for enabled stops
        if (!hasVoices) {
            for (int note = 0; note < missingNotes.size(); ++note) {
                if (missingNotes[note])
                    triggerVoicesForStop(stopIndex, note);
            }
        }
    }
}

void Division::updateAggregatedKeysState()
{
    _aggregatedKeysState = _keysState;

    for (const auto* division : _linkedFromDivisions) {
        for (const auto&[division, enabled] : division->_linkedDivisions) {
            if (division == this && enabled) {
                _aggregatedKeysState |= division->_aggregatedKeysState;
                break;
            }
        }
    }
}

bool Division::triggerVoicesForStop(const int stopIndex, const int note)
{
    isPositiveAndBelow(stopIndex, _stops.size());

    if (isAlreadyVoiced(stopIndex, note))
        return true;

    const auto& stop = _stops[stopIndex];

    if (!stop.isEnabled())
        return false;

    bool voiceTriggered = false;

    for (const auto& zone : stop.getZones()) {
        if (zone.isForKey(note)) {
            for (const auto rw : zone.rankwaves) {
                // Rankwave may be in the middle of construction, in this case
                // we don't trigger a voice.
                if (auto state = rw->trigger(note); state.isTriggered()) {
                    state.gain = stop.getGain();
                    state.chiffGain = stop.getChiffGain();

                    if (const auto voice = _engine.getVoicePool()->trigger(state)) {
                        voice->setStopIndex(stopIndex);
                        _activeVoices.append(voice);
                        voiceTriggered = true;
                    }
                }
            }
        }
    }

    return voiceTriggered;
}

bool Division::isAlreadyVoiced(const int stopIndex, const int note)
{
    auto* voice = _activeVoices.first();

    while (voice != nullptr) {
        if (voice->isActive() && voice->stopIndex() == stopIndex && voice->isForNote(note)) {
            return true;
        }

        voice = voice->next();
    }

    return false;
}
