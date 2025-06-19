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
// ----------------------------------------------------------------------------

#include "aeolus/engine.h"
#include "IOManager.h"
#include "EngineGlobal.h"
#include <any>


AEOLUS_NAMESPACE_BEGIN

namespace settings {
    const static char* tuningFrequency = "tuningFrequency";
    const static char* tuningTemperament = "tuningTemperament";
    const static char* mtsEnabled = "mtsEnabled";
}

Engine::Engine()
    : _sampleRate{SAMPLE_RATE_F}
    , _voicePool(*this)
    , _params{NUM_PARAMS}
    , _divisions{}
    , _sequencer{}
    , _subFrameBuffer{N_OUTPUT_CHANNELS, SUB_FRAME_LENGTH}
    , _divisionFrameBuffer{N_OUTPUT_CHANNELS, SUB_FRAME_LENGTH}
    , _voiceFrameBuffer{N_VOICE_CHANNELS, SUB_FRAME_LENGTH}
    , _remainedSamples{0}
    , _tremulantBuffer{1, SUB_FRAME_LENGTH}
    , _tremulantPhase{0.0f}
    , _convolver{}
    , _selectedIR{0}
    , _irSwitchEvents{}
    , _reverbTailCounter{0}
    , _interpolator{1.0f, N_OUTPUT_CHANNELS}
    , _midiKeyboardState{}
//    , _volumeLevel{}
    , _midiControlChannelsMask{ (1 << 16) - 1 }
    , _midiSwellChannelsMask{ (1 << 16) - 1 }
{
    populateDivisions();

    // Sequencer can be created only after the divisions have been populated.
    _sequencer = std::make_unique<Sequencer>(*this, SEQUENCER_N_STEPS);
}

void Engine::prepareToPlay(float sampleRate, int frameSize)
{
    // Make sure the stops wavetable is updated.
    EngineGlobal::getInstance().updateStops(SAMPLE_RATE_F);

    // Select the first IR for reverb by default
    setReverbIR(_selectedIR);
    _convolver.setDryWet(1.0f, 0.25f, true);

    _interpolator.setRatio(SAMPLE_RATE_F / sampleRate); // 44100 / sampleRate
    _interpolator.reset();

    _sampleRate = sampleRate;
}

void Engine::setReverbIR(int num)
{
    const auto& irs = EngineGlobal::getInstance().getIRs();

    if (num >= 0 && num < irs.irs.size()) {
        const auto& ir = irs.irs[num];
        _convolver.setLength(int(ir.waveform.getNumSamples() / dsp::Convolver::BlockSize + 1) * dsp::Convolver::BlockSize);
        _convolver.prepareToPlay(SAMPLE_RATE_F, SUB_FRAME_LENGTH); // these parameters are irrelevant
        _convolver.setZeroDelay(ir.zeroDelay);
        _convolver.setIR(ir);

        _reverbTailCounter = _convolver.length();

        _selectedIR = num;
    }
}

void Engine::postReverbIR(int num)
{
    // Anticipate the IR change that will happen later.
    // This is required for the UI to be updated correctly.
    _selectedIR = num;

    _irSwitchEvents.send({num});
}

float Engine::getReverbLengthInSeconds() const
{
    return float(_convolver.length()) * SAMPLE_RATE_R;
}

void Engine::setReverbWet(float v)
{
    _convolver.setDryWet(1.0f, v);
}

void Engine::setVolume(float v)
{
    _params[VOLUME].setValue(v);
}

void Engine::process(float* outL, float* outR, int numFrames, bool isNonRealtime)
{
    assert(outL != nullptr);
    assert(outR != nullptr);

    float* origOutL = outL;
    float* origOutR = outR;
    int origNumFrames = numFrames;

    processPendingIRSwitchEvents();
    processPendingNoteEvents();

    bool wasAudioGenerated = false;

    while (numFrames > 0)
    {
        if (_remainedSamples > 0)
        {
            const int idx = SUB_FRAME_LENGTH - _remainedSamples;
            const float* subL = _subFrameBuffer.getReadPointer(0, idx);
            const float* subR = _subFrameBuffer.getReadPointer(1, idx);

            while (_remainedSamples > 0 && _interpolator.canWrite()) {
                _interpolator.write(*subL, *subR);
                --_remainedSamples;
                subL += 1;
                subR += 1;
            }

            while (numFrames > 0 && _interpolator.canRead()) {
                _interpolator.read(*outL, *outR);
                numFrames -= 1;
                outL += 1;
                outR += 1;
            }
        }

        if (_remainedSamples == 0 && numFrames > 0)
        {
            wasAudioGenerated |= processSubFrame();
            assert(_remainedSamples > 0);
        }
    }

    // When there is no audio generated we let the reverb tail to
    // sound and stop the reverb processing to avoid convolving with silence.
    if (wasAudioGenerated)
        _reverbTailCounter = _convolver.length();
    else
        _reverbTailCounter = std::max(0, _reverbTailCounter - origNumFrames);

    if (_reverbTailCounter > 0 && _convolver.isAudible()) {
        _convolver.setNonRealtime(isNonRealtime);
        _convolver.process(origOutL, origOutR, origOutL, origOutR, origNumFrames);
    }

    applyVolume(origOutL, origOutR, origNumFrames);

    _volumeLevel.left.process(origOutL, origNumFrames);
    _volumeLevel.right.process(origOutR, origNumFrames);
}

void Engine::process(AudioBuffer& out, bool isNonRealtime)
{

    const int numChannels = out.getNumChannels();
    int numFrames = out.getNumSamples();

    processPendingIRSwitchEvents();
    processPendingNoteEvents();

    bool wasAudioGenerated = false;

    int outIdx = 0;

    while (numFrames > 0) {
        int idx = SUB_FRAME_LENGTH - _remainedSamples;

        while (_remainedSamples > 0 && _interpolator.canWrite()) {
            for (int ch = 0; ch < numChannels; ++ch)
                _interpolator.writeUnchecked(_subFrameBuffer.getReadPointer(ch)[idx], (size_t) ch);

            _interpolator.writeIncrement();

            _remainedSamples -= 1;
            idx += 1;
        }

        while (numFrames > 0 && _interpolator.canRead()) {
            for (int ch = 0; ch < numChannels; ++ch)
                out.getWritePointer(ch)[outIdx] = _interpolator.readUnchecked(ch);

            _interpolator.readIncrement();

            numFrames -= 1;
            outIdx += 1;
        }

        if (_remainedSamples == 0 && numFrames > 0)
        {
            wasAudioGenerated |= processSubFrame();
            assert(_remainedSamples > 0);
        }

    }

    // Multibus processing does not have a convolver FX

    // Global volume across all the buses
    applyVolume(out);

    _volumeLevel.left.process(out);
    _volumeLevel.right = _volumeLevel.left;
}

void Engine::processMIDIMessage(const MidiMessage& message)
{
    // Process global CCs
    if (midi::matchChannelToMask(getMIDIControlChannelsMask(), message.getChannel())) {
        processControlMIDIMessage(message);
    }

    if (message.isController()) {
        // Process divisions CCs
        for (auto &division : _divisions)
            division.handleControlMessage(message);
    } else if (message.isNoteOnOrOff()) {
        // Notes on/off
        _midiKeyboardState.processNextMidiEvent(message);
        return;
    }
}

void Engine::noteOn(int note, int midiChannel)
{
    clearDivisionsTriggerFlag();

    bool handled{ false };

    // Handle key switches
    // @note If a key switch falls within the playable range we need to make
    //       sure we don't process corresponding note-on event, otherwise
    //       navigating the sequencer will create a spurious sounds.
    if (midi::matchChannelToMask(getMIDIControlChannelsMask(), midiChannel)) {
        if (isKeySwitchBackward(note)) {
            _sequencer->stepBackward();
            handled = true;
        } else if (isKeySwitchForward(note)) {
            _sequencer->stepForward();
            handled = true;
        }
    }

    // Handle keys
    if (!handled) {

        // Ignore note-on event if filtered by MTS.
        auto* g = aeolus::EngineGlobal::getInstance();

        if (!g->shouldMTSFilterNote(note, midiChannel)) {
            for (auto &division : _divisions)
                division.noteOn(note, midiChannel);
        }
    }
}

void Engine::noteOff(int note, int midiChannel)
{
    clearDivisionsTriggerFlag();

    for (auto &division : _divisions) {
        division.noteOff(note, midiChannel);
    }
}

void Engine::allNotesOff()
{
    for (auto &division : _divisions)
        division.allNotesOff();

    _midiKeyboardState.allNotesOff(0);
}

Range Engine::getMidiKeyboardRange() const
{
    int minNote = -1;
    int maxNote = -1;

    for (auto &division : _divisions) {
        int min, max;
        division.getAvailableRange(min, max);

        if (min >= 0 && max >= 0) {
            if (minNote < 0 || minNote > min)
                minNote = min;

            if (maxNote < 0 || maxNote < max)
                maxNote = max;
        }
    }

    return Range(minNote, maxNote);
}

std::set<int> Engine::getKeySwitches() const
{
    std::set<int> keySwitches{};

    for (int key : _sequencerStepBackwardKeySwitches)
        keySwitches.insert(key);

    for (int key : _sequencerStepForwardKeySwitches)
        keySwitches.insert(key);

    return keySwitches;
}

Division* Engine::getDivisionByName(const std::string& name)
{
    for (auto &division : _divisions) {
        if (division.getName() == name)
            return &division;
    }

    return nullptr;
}

void Engine::clearDivisionsTriggerFlag() {
    for (auto& division : _divisions) {
        division.clearTriggerFlag();
    }
}

void Engine::postNoteEvent(bool onOff, int note, int midiChannel) {
    _pendingNoteEvents.send({onOff, note, midiChannel});
}

bool Engine::processSubFrame() {
    assert(_subFrameBuffer.getNumChannels() == _divisionFrameBuffer.getNumChannels());
    assert(_subFrameBuffer.getNumSamples() == _divisionFrameBuffer.getNumSamples());

    generateTremulant();

    _subFrameBuffer.clear();

    bool wasAudioGenerated = false;

    for (auto &division : _divisions) {

        _divisionFrameBuffer.clear();

        const bool hasVoices = division->process(_divisionFrameBuffer, _voiceFrameBuffer);
        wasAudioGenerated |= hasVoices;

        if (!hasVoices) {
            division->modulate(_divisionFrameBuffer, _tremulantBuffer);

            for (int ch = 0; ch < _subFrameBuffer.getNumChannels(); ++ch) {
                _subFrameBuffer.addFrom(ch, 0, _divisionFrameBuffer, ch, 0, SUB_FRAME_LENGTH);
            }
        }

//#if AEOLUS_MULTIBUS_OUTPUT
//        division.volumeLevel().left.process(_divisionFrameBuffer);
//        division.volumeLevel().right = division->volumeLevel().left;
//#else
//        division.volumeLevel().left.process(_divisionFrameBuffer, 0);
//        division.volumeLevel().right.process(_divisionFrameBuffer, 1);
//#endif
    }

    _remainedSamples = SUB_FRAME_LENGTH;

    return wasAudioGenerated;
}

void Engine::processPendingNoteEvents()
{
    NoteEvent event;

    while (_pendingNoteEvents.receive(event)) {
        if (event.on)
            noteOn(event.note, event.midiChannel);
        else
            noteOff(event.note, event.midiChannel);
    }
}

void Engine::processPendingIRSwitchEvents()
{
    IRSwithEvent event;
    bool received = false;

    while (_irSwitchEvents.receive(event)) {
        received = true;
    }

    if (received) {
        setReverbIR(event.num);
    }
}

void Engine::generateTremulant()
{
    float* buf = _tremulantBuffer.getWritePointer(0);
    assert(buf != nullptr);

    for (int i = 0; i < SUB_FRAME_LENGTH; ++i) {
        const float s = sinf(_tremulantPhase);
        buf[i] = s * TREMULANT_LEVEL;
        _tremulantPhase += TREMULANT_PHASE_INCREMENT;

        if (_tremulantPhase >= M_PI * 2)
            _tremulantPhase -= M_PI * 2;
    }
}

void Engine::applyVolume(AudioBuffer& out)
{
    if (_params[VOLUME].isSmoothing()) {
        for (int i = 0; i < out.getNumSamples(); ++i) {
            const float g = _params[VOLUME].nextValue() * VOLUME_GAIN;

            for (int ch = 0; ch < out.getNumChannels(); ++ch)
                out.getWritePointer(ch)[i] *= g;
        }
    } else {
        const float g = _params[VOLUME].target() * VOLUME_GAIN;
        out.applyGain(g);
    }
}

void Engine::applyVolume(float* outL, float* outR, int numFrames)
{
    if (_params[VOLUME].isSmoothing()) {
        for (int i = 0; i < numFrames; ++i) {
            const float g = _params[VOLUME].nextValue() * VOLUME_GAIN;
            outL[i] *= g;
            outR[i] *= g;
        }
    } else {
        const float g = _params[VOLUME].target() * VOLUME_GAIN;

        for (int i = 0; i < numFrames; ++i) {
            outL[i] *= g;
            outR[i] *= g;
        }
    }
}

void Engine::processControlMIDIMessage(const MidiMessage& message)
{
    if (message.isProgramChange()) {
        int step = message.getProgramChangeNumber();

        if (step >= 0 && step < _sequencer->getStepsCount())
            _sequencer->setStep(step);
    } else if (message.isController() && message.getControllerNumber() == CC_STOP_BUTTONS) {
        const auto value{ message.getControllerValue() };

        if ((value & 0xC8) == 0x40) {
            // 01mm0ggg
            StopControlMode mode { StopControlMode::Disabled };

            const int modeValue{ (value >> 4) & 0x03 };
            switch (modeValue) {
                case 0: mode = StopControlMode::Disabled; break;
                case 1: mode = StopControlMode::SetOff; break;
                case 2: mode = StopControlMode::SetOn; break;
                case 3: mode = StopControlMode::Toggle; break;
                default: break;
            }

            _stopControlMode = mode;
            _stopControlGroup = value & 0x07;

            if (_stopControlMode == StopControlMode::Disabled) {
                // Disable message does not require a 2nd part and can be processed immeditely.
                processStopControlMessage();

                _stopControlMode.reset();
            }
        } else if ((value & 0xE0) == 0) {
            // 000bbbbb
            if (_stopControlMode.has_value()) {
                _stopControlButton = value & 0x1F;

                processStopControlMessage();
            }
        } else {
            _stopControlMode.reset();
        }
    }
}

void Engine::processStopControlMessage()
{
    if (!_stopControlMode.has_value())
        return;

    if (!isPositiveAndBelow(_stopControlGroup, _divisions.size()))
        return;

    auto division{ _divisions[_stopControlGroup] };

    const auto mode{ *_stopControlMode };

    switch (mode) {
        case StopControlMode::Disabled:
            division->disableAllStops();
            break;
        case StopControlMode::SetOff:
            division->enableStop(_stopControlButton, false);
            break;
        case StopControlMode::SetOn:
            division->enableStop(_stopControlButton, true);
            break;
        case StopControlMode::Toggle:
            division->enableStop(_stopControlButton, !division->isStopEnabled(_stopControlButton));
            break;
        default:
            break;
    }
}

bool Engine::isKeySwitchForward(int key) const
{
    return std::find(_sequencerStepForwardKeySwitches.begin(),
                     _sequencerStepForwardKeySwitches.end(),
                     key) != _sequencerStepForwardKeySwitches.end();
}

bool Engine::isKeySwitchBackward(int key) const
{
    return std::find(_sequencerStepBackwardKeySwitches.begin(),
                     _sequencerStepBackwardKeySwitches.end(),
                     key) != _sequencerStepBackwardKeySwitches.end();
}

void Engine::populateDivisions() {
    const std::filesystem::path configFile = "./Resources/configs/default_organ.json";

    if (!exists(configFile))
        return;

    std::ifstream stream(configFile);
    auto config = nlohmann::json::parse(stream);

    for (auto divisionDef : config["divisions"]) {
        auto division = std::make_shared<aeolus::Division>(*this);
        division->initFromJson(divisionDef);
        _divisions.push_back(division);
    }

    if (auto sequencer = config["sequencer"]) {
        if (sequencer.contains("backward_key")) {
            populateKeySwitchesVector(_sequencerStepBackwardKeySwitches, sequencer["backward_key"]);
        }

        if (sequencer.contains("forward_key")) {
            populateKeySwitchesVector(_sequencerStepForwardKeySwitches, sequencer["forward_key"]);
        }
    }

    // Remove all the links if any.
    for (auto division : _divisions) {
        division->clearLinkedDivisions();
    }

    // Update division links after they've been loaded.
    for (auto division : _divisions) {
        division->populateLinkedDivisions();
    }

    // @todo Do we want the divisions to be reordered by the couplings?
}

void Engine::populateKeySwitchesVector(std::vector<int>& switches, const nlohmann::json& v) {
    if (v.is_null())
        return;

    switches.clear();

    if (v.is_number_integer()) {
        switches.push_back((int)v);
    } else if (v.is_array()) {
        for (const auto& key : v)
            switches.push_back(key);
    }
}

AEOLUS_NAMESPACE_END
