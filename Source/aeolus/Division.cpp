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

Division::Division(const Engine& engine, const std::string& name) : _name{name}, _mnemonic{name},
    _hasSwell{false}, _hasTremulant{false}, _midiChannelsMask{ (1 << 16) - 1 },
    _tremulantEnabled{false} /* Select all MIDI channels by default */, _tremulantLevel{0.0f},
    _tremulantMaxLevel{TREMULANT_TARGET_LEVEL}, _tremulantTargetLevel{0.0f}, _params{NUM_PARAMS},
    _swellFilterSpec{dsp::BiquadFilter::LowPass, 0.4f * SAMPLE_RATE_F, 0.7071f, 0.0f},
    _swellFilterStateL{}, _swellFilterStateR{}, _triggerFlag{}, _engine{engine} {
    dsp::BiquadFilter::updateSpec(_swellFilterSpec);
    dsp::BiquadFilter::resetState(_swellFilterSpec, _swellFilterStateL);
    dsp::BiquadFilter::resetState(_swellFilterSpec, _swellFilterStateR);
}

void Division::clearLinkedDivisions() {
    _linkedDivisions.clear();
    _linkedFromDivisions.clear();
}

void Division::populateLinkedDivisions() {
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

void Division::enableLink(const int &i, const bool &ena)
{
    isPositiveAndBelow(i, _linkedDivisions.size());

    if (_linkedDivisions[i].enabled != ena) {
        _linkedDivisions[i].enabled = ena;
        _engine.getSequencer().setCurrentStepDirty();
    }
}

bool Division::isLinkEnabled(const int &i) const {
    isPositiveAndBelow(i, _linkedDivisions.size());
    return _linkedDivisions[i].enabled;
}

Division::Link& Division::getLinkByIndex(const int &i)
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

Stop& Division::addRankwave(Rankwave *ptr, const bool &ena, const std::string& name) {
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

bool Division::isStopEnabled(const int &i) const
{
    isPositiveAndBelow(i, _stops.size());
    return _stops[i].isEnabled();
}

Stop& Division::getStopByIndex(const int &i)
{
    isPositiveAndBelow(i, _stops.size());
    return _stops[i];
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

bool Division::isForMIDIChannel(const int &channel) const noexcept
{
    const int mask{ _midiChannelsMask.load() };
    return midi::matchMidiChannelToMask(mask, channel);
}

void Division::setTremulantEnabled(const bool& ena) noexcept
{
    if (!_hasTremulant)
        return;

    if (_tremulantEnabled != ena) {
        _tremulantEnabled = ena;
        _tremulantTargetLevel = _tremulantEnabled ? _tremulantMaxLevel : 0.0f;

        _engine.getSequencer().setCurrentStepDirty();
    }
}

float Division::getTremulantLevel(const bool &update)
{
    const auto level = _tremulantLevel;

    if (update)
        _tremulantLevel += 0.1f * (_tremulantTargetLevel - _tremulantLevel);

    return level;
}

void Division::setNoteOn(const int& note, const bool& isLinkedDivision) {
    if (hasBeenTriggered())
        return;

    _triggerFlag = true;

    for (int stopIndex = 0; stopIndex < static_cast<int>(_stops.size()); ++stopIndex)
        triggerVoicesForStop(stopIndex, note);

    if (isLinkedDivision) {
        return;
    }

    // Update keys state
    _keysState.set(note);

    // Forward to the linked divisions
    for (auto&[division, enabled] : _linkedDivisions) {
        if (enabled) {
            division->setNoteOn(note, true);
        }
    }
}

void Division::setNoteOff(const int& note, const bool& isLinkedDivision) {
    if (hasBeenTriggered())
        return;

    _triggerFlag = true;

    for (const auto& voice : _activeVoices) {
        if (voice->isForNote(note) && !_keysState[note]) {
            voice->release();
        }
    }

    if (isLinkedDivision) {
        return;
    }

    // Update keys state
    _keysState.reset(note);

    // Forward to the linked divisions
    for (auto&[division, enabled] : _linkedDivisions) {
        if (enabled) {
            division->setNoteOff(note, true);
        }
    }
}

void Division::setAllNotesOff(const bool& isLinkedDivision) {
    if (!isLinkedDivision) {
        _keysState.reset();
    }
    for (const auto& voice : _activeVoices) {
        if ((isLinkedDivision && !_keysState[voice->getNote()]) || !isLinkedDivision) {
            voice->release();
        }
    }
}

void Division::handleSwell(const int& value) {
    if (_hasSwell) {
        _params[GAIN].setValue(value);
    }
}
void Division::handleTremulant(const float& value) {
    if (hasTremulant()) {
        setTremulantEnabled(value);
    }
}

void Division::setStopOn(const int& stop) {
    isPositiveAndBelow(stop, _stops.size());

    if (!_stops[stop].isEnabled()) {
        _stops[stop].setEnabled(true);
        _engine.getSequencer().setCurrentStepDirty();
    }
}

void Division::setStopOff(const int& stop) {
    isPositiveAndBelow(stop, _stops.size());

    if (_stops[stop].isEnabled()) {
        _stops[stop].setEnabled(false);
        _engine.getSequencer().setCurrentStepDirty();
    }
}

void Division::setStopToggle(const int& stop) {
    isPositiveAndBelow(stop, _stops.size());
    _stops[stop].setEnabled(!_stops[stop].isEnabled());
    _engine.getSequencer().setCurrentStepDirty();
}

void Division::setAllStopsOff() {
    for (auto& stop : _stops) {
        if (stop.isEnabled()) {
            stop.setEnabled(false);
        }
    }
}
void Division::setAllStopsOn() {
    for (auto& stop : _stops) {
        if (!stop.isEnabled()) {
            stop.setEnabled(false);
        }
    }
}

bool Division::process(StaticAudioBuffer<AUDIO_SUB_FRAME_LENGTH, OUTPUT_CHANNELS>& targetBuffer, StaticAudioBuffer<AUDIO_SUB_FRAME_LENGTH, OUTPUT_CHANNELS>& voiceBuffer) {
    updateAggregatedKeysState();
    releaseVoicesOfDisabledStops();
    triggerVoicesOfEnabledStops();

    for (auto i = _activeVoices.begin(); i != _activeVoices.end();) {
        voiceBuffer.clear();
        float* outL = voiceBuffer.getWritePointer(0);
        float* outR = voiceBuffer.getNumChannels() > 1 ? voiceBuffer.getWritePointer(1) : outL;
        (*i)->process(outL, outR);
        targetBuffer.addFrom(0, 0, voiceBuffer, 0, 0, AUDIO_SUB_FRAME_LENGTH);
        targetBuffer.addFrom(1, 0, voiceBuffer, 1, 0, AUDIO_SUB_FRAME_LENGTH);
        if ((*i)->isOver()) {
            (*i)->resetAndReturnToPool();
            i = _activeVoices.erase(i);
        } else {
            ++i;
        }
    }

    return true;
}

void Division::modulate(StaticAudioBuffer<AUDIO_SUB_FRAME_LENGTH, OUTPUT_CHANNELS>& targetBuffer, const StaticAudioBuffer<AUDIO_SUB_FRAME_LENGTH, 1>& tremulantBuffer) {
    const float* gain = tremulantBuffer.getReadPointer(0);
    const float lvl = getTremulantLevel(true);

    // Update gain smoothly
    _params[GAIN].setValue(_paramGain->value());

    float* outL = targetBuffer.getWritePointer(0);
    float* outR = targetBuffer.getWritePointer(1);

    for (int i = 0; i < AUDIO_SUB_FRAME_LENGTH; ++i) {
        _tremulantDelayL.write(outL[i]);
        _tremulantDelayR.write(outR[i]);

        const float g = (1.0f + gain[i] * lvl) * _params[GAIN].nextValue();

        constexpr float freqModCenter = TREMULANT_DELAY_LENGTH * 0.5f;
        constexpr float freqModAmp = TREMULANT_DELAY_LENGTH * 0.5f * TREMULANT_DELAY_MODULATION_LEVEL;

        const float p = freqModCenter + freqModAmp * (0.5f - gain[i] * lvl);
        outL[i] = _tremulantDelayL.read(p) * g;
        outR[i] = _tremulantDelayR.read(p) * g;
    }

    // Apply swell filter
    if (hasSwell()) {
        // Close the filter along with the gain
        const float k = powf(limitRange(0.0f, 1.0f, _params[GAIN].target()), 1.3f);
        _swellFilterSpec.freq = 400.0f + k * (18000.0f - 400.0f);
        dsp::BiquadFilter::updateSpec(_swellFilterSpec);
        dsp::BiquadFilter::process(_swellFilterSpec, _swellFilterStateL, outL, outL, AUDIO_SUB_FRAME_LENGTH);
        dsp::BiquadFilter::process(_swellFilterSpec, _swellFilterStateR, outR, outR, AUDIO_SUB_FRAME_LENGTH);
    }
}

void Division::releaseVoicesOfDisabledStops() {
    for (auto& voice : _activeVoices) {
        if (voice->isActive() && isPositiveAndBelow(voice->stopIndex(), _stops.size()) && (!_stops[voice->stopIndex()].isEnabled() || (voice->getNote() >= 0 && !_aggregatedKeysState[voice->getNote()]))) {
            voice->release();
        }
    }
}

void Division::triggerVoicesOfEnabledStops() {
    if (_aggregatedKeysState.none()) {
        // No keys are pressed
        return;
    }

    std::bitset missingNotes{ _aggregatedKeysState };

    for (int stopIndex = 0; stopIndex < _stops.size(); ++stopIndex) {
        if (!_stops[stopIndex].isEnabled()) {
            continue;
        }

        bool hasVoices = false;

        for (const auto& voice : _activeVoices) {
            if (auto note = voice->getNote(); voice->stopIndex() == stopIndex && note >= 0 && _aggregatedKeysState[note]) {
                hasVoices = true;
                missingNotes[note] = false;
                break;
            }
        }

        if (hasVoices) {
            continue;
        }

        // Trigger voices for enabled stops
        for (int note = 0; note < missingNotes.size(); ++note) {
            if (!missingNotes[note]) {
                continue;
            }
            triggerVoicesForStop(stopIndex, note);
        }
    }
}

/**
 * @brief Calculates the key state for the division by adding linked divisions' keystates to this division's keystate.
 */
void Division::updateAggregatedKeysState() {
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

bool Division::triggerVoicesForStop(const int stopIndex, const int note) {
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
                // we don't trigger a voice->
                if (auto state = rw->trigger(note); state.isTriggered()) {
                    state.gain = stop.getGain();
                    state.chiffGain = stop.getChiffGain();

                    if (const auto voice = _engine.getVoicePool()->trigger(state)) {
                        voice->setStopIndex(stopIndex);
                        _activeVoices.emplace_back(voice);
                        voiceTriggered = true;
                    }
                }
            }
        }
    }

    return voiceTriggered;
}

bool Division::isAlreadyVoiced(const int stopIndex, const int note) {
    return std::ranges::any_of(_activeVoices, [&](const auto& voice) { return voice->isActive() && voice->stopIndex() == stopIndex && voice->isForNote(note); });
}
