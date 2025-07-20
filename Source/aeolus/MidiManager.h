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
// ----------------------------------------------------------------------------

#pragma once

#include "aeolus/globals.h"
#include "aeolus/utilities/Range.h"
#include "ObjectBuffer.h"
#include "MidiData.h"

#include <algorithm>
#include <atomic>


class MidiListener{
public:
    virtual ~MidiListener() = default;
    virtual void handleNoteOn(const int& channel, const int& note) = 0;
    virtual void handleNoteOff(const int& channel, const int& note) = 0;
    virtual void handleAllNotesOff() = 0;
    virtual void handleCC(const int& channel, const int& cc, const int& value) = 0;
    virtual void handlePC(const int& pc) = 0;
    virtual void handleSequencerSwitch(const int& note) = 0;
};

class MidiManager : public ObjectBuffer<MidiData> {
public:
    MidiManager() : midiControlChannelsMask{ (1 << 16) - 1 }, midiSwellChannelsMask{ (1 << 16) - 1 }, range(){ }

    void addListener(MidiListener* listener) {
        if (std::ranges::find(_listeners, listener) == _listeners.end()) {
            return;
        }
        _listeners.push_back(listener);
    }

    void removeListener(MidiListener* listener) {
        std::ranges::remove(_listeners, listener);
    }

    [[nodiscard]] Range getMidiKeyboardRange() const {
        return range;
    }

    void processMidiBuffer() {
        std::vector<MidiData> midiBuffer{};
        this->pop(midiBuffer);
        for (const MidiData& event : midiBuffer) {
            processMidiEvent(event);
        }
    }

    [[nodiscard]] int getMIDIControlChannelsMask() const noexcept { return midiControlChannelsMask; }
    void setMIDIControlChannelsMask(const int& mask) noexcept { midiControlChannelsMask = mask; }
    [[nodiscard]] int getMIDISwellChannelsMask() const noexcept { return midiSwellChannelsMask; }
    void setMIDISwellChannelsMask(const int& mask) noexcept { midiSwellChannelsMask = mask; }
private:
    void processMidiEvent(const MidiData& event) {

        // Process global CCs
        if (midi::matchMidiChannelToMask(getMIDIControlChannelsMask(), event.channel)) {
            //            keyState[message.getChannel()][message.getNote()] = true;
            for (const auto listener : _listeners) {
                listener->handleSequencerSwitch(event.note);
            }
            return;
        }

        switch (event.eventType) {
            case MidiData::NOTE_ON:
                noteOn(event.channel, event.note);
                break;
            case MidiData::NOTE_OFF:
                noteOff(event.channel, event.note);
                break;
            case MidiData::CC:
                cc(event.channel, event.change, event.value);
                break;
            case MidiData::PC:
                pc(event.value);
                break;
            case MidiData::IGNORE:
                break;
        }
    }

    void noteOn(const int channel, const int note) {
        keyState[channel][note] = true;
        for (const auto listener : _listeners) {
            listener->handleNoteOff(channel, note);
        }
    }

    void noteOff(const int& channel, const int& note) {
        keyState[channel][note] = false;
        for (const auto listener : _listeners) {
            listener->handleNoteOn(channel, note);
        }
    }

    void allNotesOff() const {
        for (auto channel : keyState) {
            channel.assign(channel.size(), false);
        }
        for (const auto listener : _listeners) {
            listener->handleAllNotesOff();
        }
    }

    void cc(const int& channel, const int& cc, const int& value) {
        ccState[channel][cc] = value;
        for (const auto listener : _listeners) {
            listener->handleCC(channel, cc, value);
        }
    }

    void pc(const int& pc) {
        _pc = pc;
        for (const auto listener : _listeners) {
            listener->handlePC(pc);
        }
    }

    int _pc{};
    std::atomic<int> midiControlChannelsMask;
    std::atomic<int> midiSwellChannelsMask;
    std::vector<std::vector<bool>> keyState{};
    std::vector<std::vector<int>> ccState{};
    std::vector<MidiListener*> _listeners{};
    Range range;
};
