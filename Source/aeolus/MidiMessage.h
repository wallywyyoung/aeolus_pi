//
// Created by Wally Young on 6/6/25.
//

#ifndef AEOLUS_PI_MIDIMESSAGE_H
#define AEOLUS_PI_MIDIMESSAGE_H


struct MidiMessage {
    enum EventType {
        NOTE_ON, NOTE_OFF, CC, PC, IGNORE
    };

    [[nodiscard]] const int& getControllerNumber() const {
        return controllerNumber;
    }

    [[nodiscard]] const int& getControllerValue() const {
        return controllerValue;
    }

    [[nodiscard]] const int& getChannel() const {
        return channel;
    }

    [[nodiscard]] const int& getProgramNumber() const {
        return programNumber;
    }

    [[nodiscard]] const bool& isProgramChange() const {
        return eventType == EventType::PC;
    }

    [[nodiscard]] const unsigned char& getNote() const {
        return note;
    }

    [[nodiscard]] const bool& isController() const {
        return eventType == EventType::CC;
    }

    [[nodiscard]] const bool& isNoteOnOrOff() const {
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


#endif //AEOLUS_PI_MIDIMESSAGE_H
