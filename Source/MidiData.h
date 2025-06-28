
#pragma once

#include <asoundlib.h>

struct MidiData {
    enum EventType {
        NOTE_ON, NOTE_OFF, CC, IGNORE
    };
    int channel, cc, value;
    unsigned char note;
    EventType eventType;
    MidiData() = default;
    MidiData(const snd_seq_event_t& event) {
        event.type == SND_SEQ_EVENT_NOTEON;
        switch(event.type) {
            case SND_SEQ_EVENT_NOTEON:
                eventType = NOTE_ON;
                channel = event.data.note.channel;
                note = event.data.note.note;
                break;
            case SND_SEQ_EVENT_NOTEOFF:
                eventType = NOTE_OFF;
                channel = event.data.note.channel;
                note = event.data.note.note;
                break;
            case SND_SEQ_EVENT_CONTROLLER:
                eventType = CC;
                cc = event.data.control.param;
                value = event.data.control.value;
                break;
            default:
                eventType = IGNORE;
        };
    }
};