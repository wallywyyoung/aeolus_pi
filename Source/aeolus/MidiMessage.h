// ----------------------------------------------------------------------------
//
//  Copyright (C) 2025 Wally Young <wallywyyoung@users.noreply.github.com>
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

struct MidiMessage {
    enum EventType {
        NOTE_ON, NOTE_OFF, CC, PC, IGNORE
    };

    MidiMessage(const int controller_number, const int controller_value, const int channel, const int program_number, const unsigned char note,
        const EventType event_type)
        : controllerNumber(controller_number),
          controllerValue(controller_value),
          channel(channel),
          programNumber(program_number),
          note(note),
          eventType(event_type) {
    }

    [[nodiscard]] const int& getControllerNumber() const {
        return controllerNumber;
    }

    [[nodiscard]] const int& getControllerValue() const {
        return controllerValue;
    }

    [[nodiscard]] const int& getChannel() const {
        return channel;
    }

    [[nodiscard]] int getProgramNumber() const {
        return programNumber;
    }

    [[nodiscard]] bool isProgramChange() const {
        return eventType == EventType::PC;
    }

    [[nodiscard]] const unsigned char& getNote() const {
        return note;
    }

    [[nodiscard]] bool isController() const {
        return eventType == EventType::CC;
    }

    [[nodiscard]] bool isNoteOnOrOff() const {
        return eventType == EventType::NOTE_ON || EventType::NOTE_OFF;
    }

    [[nodiscard]] const EventType& getEventType() const {
        return eventType;
    }

private:
    int controllerNumber, controllerValue, channel, programNumber;
    unsigned char note;
    EventType eventType;
};
