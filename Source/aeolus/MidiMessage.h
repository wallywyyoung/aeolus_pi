//
// Created by Wally Young on 6/6/25.
//

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
