// ----------------------------------------------------------------------------
//
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
#include "aeolus/division.h"
#include "aeolus/engine.h"
#include "aeolus/EngineGlobal.h"
#include "Configuration.h"

Division::Division(const Engine& engine, const Configuration& config, const std::string& name)
    : configuration(config)
    , _name{name}
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

void Division::initFromJson(const nlohmann::json& v)
{
        _name = v["name"];
        _mnemonic = v["mnemonic"];

        const auto link = v["link"];

        if (link.is_array()) {
            for (const auto& item : link)
                _linkedDivisionNames.push_back(item);
        }

        _hasSwell = v["swell"];
        _hasTremulant = v["tremulant"];

        _tremulantMaxLevel = 0.0f;

        if (_hasTremulant)
            _tremulantMaxLevel = v["tremulant_level"];

        const auto arr = v["stops"];
        if (arr.is_array()) {
            for (int i = 0; i < arr.size(); ++i) {
                Stop stop { configuration };
                stop.initFromJson(arr[i]);

                if (!stop.getZones().empty())
                    _stops.push_back(stop);
            }
        }
    }
//
//var Division::getPersistentState() const
//{
//    auto* divisionObj = new DynamicObject();
//
//    divisionObj->setProperty("midi_channels_mask", getMIDIChannelsMask());
//    divisionObj->setProperty("tremulant_enabled", isTremulantEnabled());
//
//    {
//        Array<var> stops;
//
//        for (const auto& stop : _stops) {
//            auto* stopObj = new DynamicObject();
//            stopObj->setProperty("name", stop.getName());
//            stopObj->setProperty("enabled", stop.isEnabled());
//
//            stops.add(var{stopObj});
//        }
//
//        divisionObj->setProperty("stops", stops);
//    }
//
//    {
//        Array<var> links;
//
//        for (const auto& link : _linkedDivisions) {
//            auto* linkObj = new DynamicObject();
//            linkObj->setProperty("division", link.division->getName());
//            linkObj->setProperty("enabled", link.enabled);
//
//            links.add(var{linkObj});
//        }
//
//        divisionObj->setProperty("links", links);
//    }
//
//    return var{divisionObj};
//}
//
//void Division::setPersistentState(const std::map<std::string, std::any>& v)
//{
//    if (const auto* divisionObj = v.getDynamicObject()) {
//
//        if (const auto& v = divisionObj->getProperty("midi_channel"); !v.isVoid()) {
//            // Handle legacy setting with only one MIDI channel allowed
//            const int channel{ (int)v };
//
//            if (channel == 0)
//                setMIDIChannelsMask((1 << 16) - 1); // Select all MIDI channels
//            else
//                setMIDIChannelsMask(1 << (channel - 1));
//        } else {
//            setMIDIChannelsMask(divisionObj->getProperty("midi_channels_mask"));
//        }
//
//        setTremulantEnabled(divisionObj->getProperty("tremulant_enabled"));
//
//        if (const auto* stops = divisionObj->getProperty("stops").getArray()) {
//            for (int i = 0; i < stops->size(); ++i) {
//                if (const auto* stopObj = stops->getReference(i).getDynamicObject()) {
//                    const String stopName = stopObj->getProperty("name");
//                    const bool enabled = stopObj->getProperty("enabled");
//
//                    for (auto& stop : _stops) {
//                        if (stop.getName()== stopName) {
//                            stop.setEnabled(enabled);
//                            break;
//                        }
//                    }
//                }
//            }
//        }
//
//        if (const auto* links = divisionObj->getProperty("links").getArray()) {
//            for (int i = 0; i < links->size(); ++i) {
//                if (const auto* linkObj = links->getReference(i).getDynamicObject()) {
//                    const String divisionName = linkObj->getProperty("division");
//                    const bool enabled = linkObj->getProperty("enabled");
//
//                    for (auto& link : _linkedDivisions) {
//                        if (link.division->getName() == divisionName)
//                            link.enabled = enabled;
//                    }
//                }
//            }
//        }
//    }
//}

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

Stop& Division::addRankwave(const std::shared_ptr<Rankwave> &ptr, const bool ena, const std::string& name) {
    assert(ptr != nullptr);

    Stop ref(configuration);
    ref.addZone(ptr);
    ref.setEnabled(ena);
    ref.setName(name.empty() ? ptr->getStopName() : name);

    _stops.push_back(ref);
    return _stops.back();
}

Stop& Division::addRankwaves(const std::vector<std::shared_ptr<Rankwave>> &rw, const bool ena, const std::string& name)
{
    assert(!rw.empty());
    Stop ref(configuration);
    ref.addZone(rw);
    ref.setEnabled(ena);
    ref.setName(name.empty() ? rw[0]->getStopName() : name);

    _stops.push_back(ref);
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
    return midi::matchChannelToMask(mask, channel);
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
    for (auto& link : _linkedDivisions) {
        if (link.enabled) {
            link.division->noteOn(note, 0);
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

void Division::handleControlMessage(const MidiMessage& msg)
{
    const int cc{ msg.getControllerNumber() };

    if (cc != CC_MODULATION && cc != CC_VOLUME && cc != CC_ALL_NOTES_OFF)
        return;

    const int swellCh{ configuration.getMIDISwellChannelsMask() };
    const float value{ float(msg.getControllerValue()) / 127.0f };

    if (msg.getChannel() == 0 || (swellCh & (msg.getChannel() - 1)) != 0) {
        if (_hasSwell && cc == CC_VOLUME) {
//            *_paramGain = value;
            _params[Division::GAIN].setValue(value);
        }
    }

    // Hange manual channel specific controls
    if (!isForMIDIChannel(msg.getChannel()))
        return;

    if (cc == CC_MODULATION && hasTremulant())
        setTremulantEnabled(value > 0.5f);

    if (cc == CC_ALL_NOTES_OFF)
        allNotesOff();
}

bool Division::process(AudioBuffer& targetBuffer, AudioBuffer& voiceBuffer)
{
    assert(targetBuffer.getNumSamples() == SUB_FRAME_LENGTH);
    assert(voiceBuffer.getNumSamples() == SUB_FRAME_LENGTH);

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

void Division::modulate(AudioBuffer& targetBuffer, const AudioBuffer& tremulantBuffer)
{
    assert(targetBuffer.getNumSamples() == SUB_FRAME_LENGTH);
    assert(tremulantBuffer.getNumSamples() == SUB_FRAME_LENGTH);

    const float* gain = tremulantBuffer.getReadPointer(0);
    assert(gain != nullptr);

    const float lvl = getTremulantLevel(true);

    // Update gain smoothly
    auto& paramGain = _params[Division::GAIN];
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
            const int stopIndex = voice->stopIndex();

            if (isPositiveAndBelow(stopIndex, _stops.size())) {
                const auto& stop = _stops[stopIndex];

                if (!stop.isEnabled())
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

    std::bitset<TOTAL_NOTES> missingNotes{ _aggregatedKeysState };

    for (int stopIndex = 0; stopIndex < _stops.size(); ++stopIndex) {
        if (auto& stop = _stops[stopIndex]; !stop.isEnabled())
            continue;

        bool hasVoices = false;

        auto* voice = _activeVoices.first();

        while (voice != nullptr) {
            if (const int voiceNote{ voice->getNote() }; voice->stopIndex() == stopIndex && voiceNote >= 0 && _aggregatedKeysState[voiceNote]) {
                hasVoices = true;
                missingNotes[voiceNote] = 0;
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
            if (division.get() == this && enabled) {
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
