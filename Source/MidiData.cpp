
#include "MidiData.h"

#include <iostream>

#ifdef LINUX


MidiData::MidiData(const snd_seq_event_t &event) {
    *this = event;
}

MidiData & MidiData::operator=(const snd_seq_event_t &event) {
    switch(event.type) {
        case SND_SEQ_EVENT_NOTEON:
            eventType = event.data.note.velocity ? NOTE_ON : NOTE_OFF;
            channel = event.data.note.channel;
            param = event.data.note.note;
            break;
        case SND_SEQ_EVENT_NOTEOFF:
            eventType = NOTE_OFF;
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
            break;
    }
    return *this;
}

#endif

bool MidiData::valid() const {
    return eventType != INVALID;
}


