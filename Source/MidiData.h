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

#include <alsa/asoundlib.h>

struct MidiData {
    enum EventType : unsigned char {
        NOTE_ON = 0, NOTE_OFF = 1, CC = 2, PC = 3
    };

    EventType eventType : 2;
    unsigned char channel : 7, param : 7, value : 7;

    MidiData() = default;
    explicit MidiData(const snd_seq_event_t &event);

    MidiData &operator=(const snd_seq_event_t &event);

    bool valid() const;
};