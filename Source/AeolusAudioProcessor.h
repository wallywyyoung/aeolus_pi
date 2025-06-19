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

#pragma once

#include "aeolus/engine.h"
#include "aeolus/EngineGlobal.h"

#include "Parameters.h"

class AeolusAudioProcessor {
public:
    AeolusAudioProcessor();
    ~AeolusAudioProcessor();

    Parameters& getParametersContainer() noexcept { return _parameters; }

    void panic() noexcept { _panicRequest = true; }

    AeolusAudioProcessor* getAudioProcessor() { return this; }
    aeolus::Engine& getEngine() { return _engine; }
    void killAllVoices() { panic(); }
    int getNumberOfActiveVoices()  { return _engine.getVoiceCount(); }

    void prepareToPlay (double sampleRate, int samplesPerBlock);
    void releaseResources();

    bool canAddBus(bool isInput) const;
    bool canRemoveBus(bool isInput) const;
    bool canApplyBusCountChange(bool isInput, bool isAdding, BusProperties& outProperties);
    bool isBusesLayoutSupported(const BusesLayout& layouts) const;
    void processorLayoutsChanged();
    void processBlock(AudioBuffer& buffer, MidiBuffer& midiMessages);
    void processMidi (MidiBuffer& midiMessages);

    int getNumPrograms();
    int getCurrentProgram();
    void setCurrentProgram (int index);
    const std::string getProgramName (int index);
    void changeProgramName (int index, const std::string& newName);

    float getProcessLoad() const noexcept { return _processLoad; }
    int getActiveVoiceCount() const noexcept { return (int) _engine.getVoiceCount(); }

    void handleNoteOn(MidiKeyboardState* source, int midiChannel, int midiNoteNumber, float velocity);
    void handleNoteOff(MidiKeyboardState* source, int midiChannel, int midiNoteNumber, float velocity);

private:

    static BusesProperties getBusesProperties();

    aeolus::Engine _engine;

    Parameters _parameters;

    std::atomic<float> _processLoad;
    std::atomic<bool> _panicRequest;
};
