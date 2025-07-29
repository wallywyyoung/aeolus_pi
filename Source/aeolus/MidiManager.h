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

#include "ObjectBuffer.h"
#include "MidiData.h"
#include <atomic>

class MidiManager : public ObjectBuffer<MidiData> {
public:
    MidiManager() : ObjectBuffer(), midiControlChannelsMask{ (1 << 16) - 1 }, midiSwellChannelsMask{ (1 << 16) - 1 } { }

    virtual void handleNoteOn(const int& channel, const int& note) = 0;
    virtual void handleNoteOff(const int& channel, const int& note) = 0;
    virtual void handleAllNotesOff() = 0;
    virtual void handleCC(const int& channel, const int& cc, const int& value) = 0;
    virtual void handlePC(const int& pc) = 0;
    virtual void handleSequencerSwitch(const int& note) = 0;

    // [[nodiscard]] Range getMidiKeyboardRange() const {
    //     return range;
    // }

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
protected:
    void processMidiEvent(const MidiData& event) {
        // Process global CCs
        if (event.channel == 15) {
            handleSequencerSwitch(event.param);
            return;
        }

        switch (event.eventType) {
            case MidiData::NOTE_ON:
                handleNoteOn(event.channel, event.param);
                break;
            case MidiData::NOTE_OFF:
                handleNoteOff(event.channel, event.param);
                break;
            case MidiData::CC:
                handleCC(event.channel, event.param, event.value);
                break;
            case MidiData::PC:
                handlePC(event.value);
                break;
        }
    }

    // int _pc{};
    std::atomic<int> midiControlChannelsMask;
    std::atomic<int> midiSwellChannelsMask;
    // std::vector<std::vector<bool>> keyState{};
    // std::vector<std::vector<int>> ccState{};
};
