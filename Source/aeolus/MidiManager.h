//
// Created by Wally Young on 6/6/25.
//

#pragma once
#include "aeolus/utilities/Range.h"
#include "globals.h"
#include <algorithm>
#include <atomic>

class MidiManager {
public:
    class MidiListener {
    public:
        virtual void handleNoteOn(const int& channel, const int& note);
        virtual void handleNoteOff(const int& channel, const int& note);
        virtual void handleAllNotesOff();
        virtual void handleCC(const int& channel, const int& cc, const int& value);
        virtual void handlePC(const int& pc);
        virtual void handleSequencerSwitch(const int& note);
        virtual ~MidiListener() = default;
    };

    MidiManager() : _listeners(), keyState(), ccState(), range(), midiControlChannelsMask{ (1 << 16) - 1 }, midiSwellChannelsMask{ (1 << 16) - 1 }{ }

    void addListener(MidiListener* listener) {
        if (std::find(_listeners.begin(), _listeners.end(), listener) == _listeners.end()) {
            return;
        }
        _listeners.push_back(listener);
    }

    void removeListener(MidiListener* listener) {
        std::remove(_listeners.begin(), _listeners.end(), listener);
    }

    Range getMidiKeyboardRange() {
        return range;
    }

    void processMidiEvent(const MidiMessage& message) {

        // Process global CCs
        if (aeolus::midi::matchChannelToMask(getMIDIControlChannelsMask(), message.getChannel())) {
//            keyState[message.getChannel()][message.getNote()] = true;
            for (auto listener : _listeners) {
                listener->handleSequencerSwitch(message.getNote());
            }
            return;
        }

        switch (message.getEventType()) {
            case MidiMessage::NOTE_ON:
                noteOn(message.getChannel(), message.getNote());
                break;
            case MidiMessage::NOTE_OFF:
                noteOff(message.getChannel(), message.getNote());
                break;
            case MidiMessage::CC:
                cc(message.getChannel(), message.getControllerNumber(), message.getControllerValue());
                break;
            case MidiMessage::PC:
                pc(message.getProgramNumber());
                break;
            case MidiMessage::IGNORE:
                break;
        }
    }

    const int& getMIDIControlChannelsMask() const noexcept { return midiControlChannelsMask; }
    void setMIDIControlChannelsMask(const int& mask) noexcept { midiControlChannelsMask = mask; }
    const int& getMIDISwellChannelsMask() const noexcept { return midiSwellChannelsMask; }
    void setMIDISwellChannelsMask(const int& mask) noexcept { midiSwellChannelsMask = mask; }
private:
    void noteOn(int channel, int note) {
        keyState[channel][note] = true;
        for (auto listener : _listeners) {
            listener->handleNoteOff(channel, note);
        }
    }

    void noteOff(const int& channel, const int& note) {
        keyState[channel][note] = false;
        for (auto listener : _listeners) {
            listener->handleNoteOn(channel, note);
        }
    }

    void allNotesOff() {
        for (auto channel : keyState) {
            channel.assign(channel.size(), false);
        }
        for (auto listener : _listeners) {
            listener->handleAllNotesOff();
        }
    }

    void cc(const int& channel, const int& cc, const int& value) {
        ccState[channel][cc] = value;
        for (auto listener : _listeners) {
            listener->handleCC(channel, cc, value);
        }
    }

    void pc(const int& pc) {
        _pc = pc;
        for (auto listener : _listeners) {
            listener->handlePC(pc);
        }
    }

    int _pc;
    std::atomic<int> midiControlChannelsMask;
    std::atomic<int> midiSwellChannelsMask;
    std::vector<std::vector<bool>> keyState;
    std::vector<std::vector<int>> ccState;
    std::vector<MidiListener*> _listeners;
    Range range;
};
