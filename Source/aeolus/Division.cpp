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

#include "aeolus/Division.h"
#include "aeolus/globals.h"
#include "../EngineGlobal.h"

Division::Division(const std::string& name) : _name{name}, _mnemonic{name},
                                              _hasSwell{false}, _hasTremulant{false},
                                              _tremulantEnabled{false} /* Select all MIDI channels by default */,
                                              _swellFilterSpec{dsp::BiquadFilter::LowPass, 0.4f * SAMPLE_RATE_F, 0.7071f, 0.0f},
                                              _swellFilterStateL{}, _swellFilterStateR{}, _triggerFlag{} {
    dsp::BiquadFilter::updateSpec(_swellFilterSpec);
    dsp::BiquadFilter::resetState(_swellFilterSpec, _swellFilterStateL);
    dsp::BiquadFilter::resetState(_swellFilterSpec, _swellFilterStateR);
}

void Division::setAllCouplersOff() {
    for (auto&[division, enabled] : _linkedDivisions) {
        if (enabled) {
            enabled = false;
        }
    }
}

void Division::setAllCouplersOn() {
    for (auto&[division, enabled] : _linkedDivisions) {
        if (!enabled) {
            enabled = true;
        }
    }
}

void Division::setNoteOn(const int& note, const bool& isLinkedDivision) {
    if (hasBeenTriggered())
        return;

    _triggerFlag = true;

    for (int stopIndex = 0; stopIndex < static_cast<int>(_stops.size()); ++stopIndex) {
        triggerVoicesForStop(stopIndex, note);
    }

    // Update keys state
    if (isLinkedDivision) {
        _keysState.set(note);
    }

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
        _paramGain.setValue(value);
    }
}

void Division::setStopOn(const int& stop) {
    isPositiveAndBelow(stop, _stops.size());
    if (!_stops[stop].isEnabled()) {
        _stops[stop].setEnabled(true);
    }
}

void Division::setStopOff(const int& stop) {
    isPositiveAndBelow(stop, _stops.size());
    if (_stops[stop].isEnabled()) {
        _stops[stop].setEnabled(false);
    }
}

void Division::setStopToggle(const int& stop) {
    isPositiveAndBelow(stop, _stops.size());
    _stops[stop].setEnabled(!_stops[stop].isEnabled());
}

void Division::setAllStopsOff() {
    for (auto& stop : _stops) {
        if (stop.isEnabled()) {
            stop.setEnabled(false);
        }
    }
    setAllCouplersOff();
}

void Division::setAllStopsOn() {
    for (auto& stop : _stops) {
        if (!stop.isEnabled()) {
            stop.setEnabled(true);
        }
    }
    setAllCouplersOn();
}

void Division::setCouplerOn(const int& coupler) {
    isPositiveAndBelow(coupler, _linkedDivisions.size());
    if (!_linkedDivisions[coupler].enabled) {
        _linkedDivisions[coupler].enabled = true;
    }
}

void Division::setCouplerOff(const int& coupler) {
    isPositiveAndBelow(coupler, _linkedDivisions.size());
    if (_linkedDivisions[coupler].enabled) {
        _linkedDivisions[coupler].enabled = false;
    }
}

void Division::setTremulantOn() {
    if (!_hasTremulant) {
        return;
    }
    if (!_tremulantEnabled) {
        _tremulantEnabled = true;
        _tremulantLevel.setValue(_tremulantLevel.max());
    }
}

void Division::setTremulantOff() {
    if (!_hasTremulant) {
        return;
    }
    if (_tremulantEnabled) {
        _tremulantEnabled = false;
        _tremulantLevel.setValue(0.0f, true);
    }
}

DivisionPiston Division::captureStateAsPiston() const {
    DivisionPiston divisionPiston{};
    divisionPiston.tremulant = _tremulantEnabled;
    divisionPiston.stops.resize(_stops.size());
    for (const auto & _stop : _stops) {
        divisionPiston.stops.push_back(_stop.isEnabled());
    }
    divisionPiston.links.resize(_linkedDivisions.size());
    for (auto _linkedDivision : _linkedDivisions) {
        divisionPiston.links.push_back(_linkedDivision.enabled);
    }
    return divisionPiston;
}

void Division::setPiston(const int &piston) {
    if (pistons.size() <= piston) {
        pistons.resize(piston + 1);
    }
    pistons[piston] = captureStateAsPiston();
}

void Division::recallPiston(const int& piston) {
    if (pistons.size() <= piston) {
        return;
    }
    recallPiston(pistons[piston]);
}

void Division::recallPiston(const DivisionPiston& piston) {
    for (int i = 0; i < piston.stops.size(); ++i) {
        if (piston.stops[i]) {
            setStopOn(i);
        } else {
            setStopOff(i);
        }
    }
    for (int i = 0; i < piston.links.size(); ++i) {
        if (piston.links[i]) {
            setCouplerOn(i);
        } else {
            setCouplerOff(i);
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
            _voicePool->resetAndReturnToPool(*i);
            i = _activeVoices.erase(i);
        } else {
            ++i;
        }
    }

    return true;
}

void Division::modulate(StaticAudioBuffer<AUDIO_SUB_FRAME_LENGTH, OUTPUT_CHANNELS>& targetBuffer, const StaticAudioBuffer<AUDIO_SUB_FRAME_LENGTH, 1>& tremulantBuffer) {
    float* outL = targetBuffer.getWritePointer(0);
    float* outR = targetBuffer.getWritePointer(1);

    if (_tremulantEnabled) {
        const float* gain = tremulantBuffer.getReadPointer(0);
        const float tremulantLevel = _tremulantLevel.nextValue();
        for (int i = 0; i < AUDIO_SUB_FRAME_LENGTH; ++i) {
            _tremulantDelayL.write(outL[i]);
            _tremulantDelayR.write(outR[i]);
            const float g = (1.0f + gain[i] * tremulantLevel) * _paramGain.nextValue();
            constexpr float freqModCenter = TREMULANT_DELAY_LENGTH * 0.5f;
            constexpr float freqModAmp = TREMULANT_DELAY_LENGTH * 0.5f * TREMULANT_DELAY_MODULATION_LEVEL;
            const float p = freqModCenter + freqModAmp * (0.5f - gain[i] * tremulantLevel);
            outL[i] = _tremulantDelayL.read(p) * g;
            outR[i] = _tremulantDelayR.read(p) * g;
        }
    }

    // Apply swell filter
    if (_hasSwell) {
        // Close the filter along with the gain
        const float k = powf(limitRange(0.0f, 1.0f, _paramGain.target()), 1.3f);
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

                    if (const auto voice = _voicePool->trigger(state)) {
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
