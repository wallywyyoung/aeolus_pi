
#include "MidiData.h"

#include <iostream>

#ifdef LINUX

#include <ostream>

MidiData::MidiData(const snd_seq_event_t &event) {
    *this = event;
}

MidiData & MidiData::operator=(const snd_seq_event_t &event) {
    std::cout << "Got Event: ";
    switch(event.type) {
        case SND_SEQ_EVENT_NOTEON:
            std::cout << "Note " << (event.data.note.velocity ? "On" : "Off") << " Channel: " << event.data.note.channel << " Note: " << event.data.note.note ;
            eventType = event.data.note.velocity ? NOTE_ON : NOTE_OFF;
            channel = event.data.note.channel;
            param = event.data.note.note;
            break;
        case SND_SEQ_EVENT_NOTEOFF:
            eventType = NOTE_OFF;
            std::cout << "Note Off" << " Channel: " << event.data.note.channel << " Note: " << event.data.note.note ;
            channel = event.data.note.channel;
            param = event.data.note.note;
            break;
        case SND_SEQ_EVENT_CONTROLLER:
            eventType = CC;
            channel = event.data.control.channel;
            param = event.data.control.param;
            value = event.data.control.value;
            break;
        case SND_SEQ_EVENT_PGMCHANGE:
            eventType = PC;
            channel = event.data.control.channel;
            value = event.data.control.value;
            break;
        default:
            eventType = INVALID;
            std::cout << "Invalid?";
            break;
    }
    std::cout << std::endl;
    return *this;
}

#endif

bool MidiData::valid() const {
    std::cout << "Valid: " << (eventType!=INVALID) << std::endl;
    return eventType != INVALID;
}


