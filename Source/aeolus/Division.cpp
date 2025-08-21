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
#include "EngineGlobal.h"

Division::Division(const std::string& name) : name{name}, mnemonic{name},
                                              hasSwell{false}, hasTremulant{false},
                                              tremulantEnabled{false} /* Select all MIDI channels by default */,
                                              swellFilterSpec{dsp::BiquadFilter::LowPass, 0.4f * SAMPLE_RATE_F, 0.7071f, 0.0f},
                                              swellFilterStateL{}, swellFilterStateR{} {
    dsp::BiquadFilter::updateSpec(swellFilterSpec);
    dsp::BiquadFilter::resetState(swellFilterSpec, swellFilterStateL);
    dsp::BiquadFilter::resetState(swellFilterSpec, swellFilterStateR);
}

void Division::setAllCouplersOff() {
    for (const auto & coupler : linkedDivisions) {
        coupler->enabled = false;
    }
}

void Division::setAllCouplersOn() {
    for (const auto& coupler : linkedDivisions) {
        coupler->enabled = true;
    }
}

void Division::setNoteOn(const int &note) {
    keysState.set(note);
}

void Division::setNoteOff(const int &note) {
    keysState.reset(note);
}

void Division::setAllNotesOff() {
    keysState.reset();
}

void Division::handleSwell(const int& value) {
    if (hasSwell) {
        gain.setValue(value);
    }
}

void Division::setStopOn(const int& stop) {
    assertIsPositiveAndBelow(stop, stops.size());
    if (!stops[stop].isEnabled()) {
        stops[stop].setEnabled(true);
    }
}

void Division::setStopOff(const int& stop) {
    assertIsPositiveAndBelow(stop, stops.size());
    if (stops[stop].isEnabled()) {
        stops[stop].setEnabled(false);
    }
}

void Division::setStopToggle(const int& stop) {
    assertIsPositiveAndBelow(stop, stops.size());
    stops[stop].setEnabled(!stops[stop].isEnabled());
}

void Division::setAllStopsOff() {
    for (auto& stop : stops) {
        if (stop.isEnabled()) {
            stop.setEnabled(false);
        }
    }
    setAllCouplersOff();
}

void Division::setAllStopsOn() {
    for (auto& stop : stops) {
        if (!stop.isEnabled()) {
            stop.setEnabled(true);
        }
    }
    setAllCouplersOn();
}

void Division::setCouplerOn(const int& coupler) {
    assertIsPositiveAndBelow(coupler, linkedDivisions.size());
    if (!linkedDivisions[coupler]->enabled) {
        linkedDivisions[coupler]->enabled = true;
    }
}

void Division::setCouplerOff(const int& coupler) {
    assertIsPositiveAndBelow(coupler, linkedDivisions.size());
    if (linkedDivisions[coupler]->enabled) {
        linkedDivisions[coupler]->enabled = false;
    }
}

void Division::setTremulantOn() {
    if (!hasTremulant) {
        return;
    }
    if (!tremulantEnabled) {
        tremulantEnabled = true;
        tremulantLevel.setValue(tremulantLevel.max());
    }
}

void Division::setTremulantOff() {
    if (!hasTremulant) {
        return;
    }
    if (tremulantEnabled) {
        tremulantEnabled = false;
        tremulantLevel.setValue(0.0f, true);
    }
}

DivisionPiston Division::captureStateAsPiston() const {
    DivisionPiston divisionPiston{};
    divisionPiston.tremulant = tremulantEnabled;
    divisionPiston.stops.resize(stops.size());
    for (const auto & _stop : stops) {
        divisionPiston.stops.push_back(_stop.isEnabled());
    }
    divisionPiston.links.resize(linkedDivisions.size());
    for (const auto linkedDivision : linkedDivisions) {
        divisionPiston.links.push_back(linkedDivision->enabled);
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
    aggregatedKeysState.reset();
    auto traversed = std::vector<Division*>();
    recursiveKeyState(aggregatedKeysState, traversed);
    releaseVoicesOfDisabledStops();
    triggerVoicesOfEnabledStops();

    auto hasVoices = false;
    for (auto i = activeVoices.begin(); i != activeVoices.end();) {
        voiceBuffer.clear();
        (*i)->process(voiceBuffer);
        targetBuffer.addFrom(voiceBuffer);
        if ((*i)->isOver()) {
            (*i)->reset();
            i = activeVoices.erase(i);
        } else {
            ++i;
        }
        hasVoices = true;
    }
    return hasVoices;
}

void Division::modulate(StaticAudioBuffer<AUDIO_SUB_FRAME_LENGTH, OUTPUT_CHANNELS>& targetBuffer, const StaticAudioBuffer<AUDIO_SUB_FRAME_LENGTH, 1>& tremulantBuffer) {
    float* outL = targetBuffer.getWritePointer(0);
    float* outR = targetBuffer.getWritePointer(1);

    if (tremulantEnabled) {
        const float* tremulantGain = tremulantBuffer.getReadPointer(0);
        const float thisTremulantLevel = tremulantLevel.nextValue();
        for (auto i = 0; i < AUDIO_SUB_FRAME_LENGTH; ++i) {
            tremulantDelayL.write(outL[i]);
            tremulantDelayR.write(outR[i]);
            const float g = (1.0f + tremulantGain[i] * thisTremulantLevel) * gain.nextValue();
            constexpr float freqModCenter = TREMULANT_DELAY_LENGTH * 0.5f;
            constexpr float freqModAmp = TREMULANT_DELAY_LENGTH * 0.5f * TREMULANT_DELAY_MODULATION_LEVEL;
            const float p = freqModCenter + freqModAmp * (0.5f - tremulantGain[i] * thisTremulantLevel);
            outL[i] = tremulantDelayL.read(p) * g;
            outR[i] = tremulantDelayR.read(p) * g;
        }
    }

    // Apply swell filter
    if (hasSwell) {
        // Close the filter along with the gain
        const float k = powf(limitRange(0.0f, 1.0f, gain.target()), 1.3f);
        swellFilterSpec.freq = 400.0f + k * (18000.0f - 400.0f);
        dsp::BiquadFilter::updateSpec(swellFilterSpec);
        dsp::BiquadFilter::process(swellFilterSpec, swellFilterStateL, outL, outL, AUDIO_SUB_FRAME_LENGTH);
        dsp::BiquadFilter::process(swellFilterSpec, swellFilterStateR, outR, outR, AUDIO_SUB_FRAME_LENGTH);
    }
}

void Division::releaseVoicesOfDisabledStops() {
    for (const auto& voice : activeVoices) {
        if (!voice->isActive()) {
            continue;
        }
        if (!aggregatedKeysState[voice->getNote()]) {
            voice->release();
            continue;
        }
        for (auto stopIndex = 0; stopIndex < stops.size(); ++stopIndex) {
            if (voice->getStopIndex() == stopIndex && !stops[stopIndex].isEnabled()) {
                voice->release();
            }
        }
    }
}

void Division::triggerVoicesOfEnabledStops() {
    if (aggregatedKeysState.none()) {
        return;
    }
    for (auto note = 0; note < aggregatedKeysState.size(); ++note) {
        if (!aggregatedKeysState[note]) {
            continue;
        }
        for (auto stopIndex = 0; stopIndex < stops.size(); ++stopIndex) {
            triggerVoicesForStop(stopIndex, note);
        }
    }
}

/**
 * @brief Calculates the key state for the division by adding linked divisions' keystates to this division's keystate.
 */
void Division::recursiveKeyState(std::bitset<TOTAL_NOTES> &aggregated, std::vector<Division *> &traversed) {
    for (const auto division : traversed) {
        if (division == this) {
            return;
        }
    }
    aggregated |= keysState;
    traversed.push_back(this);
    // TODO: Consider pedal <-> manual bit state.
    for (const auto& coupler : linkedFromDivisions) {
        if (coupler->enabled) {
            coupler->division->recursiveKeyState(aggregated, traversed);
        }
    }
}

bool Division::triggerVoicesForStop(const int stopIndex, const int note) {
    const auto& stop = stops[stopIndex];

    if (!stop.isEnabled()) { return false; }
    if (isAlreadyVoiced(stopIndex, note)) { return true; }

    auto voiceTriggered = false;

    for (const auto& zone : stop.getZones()) {
        if (zone.isForKey(note)) {
            for (const auto rankWave : zone.rankWaves) {
                if (auto state = rankWave->trigger(note); state.isTriggered()) {
                    state.gain = stop.getGain();
                    state.chiffGain = stop.getChiffGain();
                    if (const auto voice = voicePool->trigger(state)) {
                        voice->setStopIndex(stopIndex);
                        activeVoices.emplace_back(voice);
                        voiceTriggered = true;
                    }
                }
            }
        }
    }

    return voiceTriggered;
}

bool Division::isAlreadyVoiced(const int stopIndex, const int note) {
    return std::ranges::any_of(activeVoices, [&](const auto& voice) { return voice->isActive() && voice->getStopIndex() == stopIndex && voice->isForNote(note); });
}
