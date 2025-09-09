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

#ifdef LINUX

#include "AlsaMidiInterface.h"
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include "MidiData.h"

using namespace std::literals::chrono_literals;

AlsaMidiInterface::AlsaMidiInterface(std::function<void(const MidiData&)> submitMidi) : midiThreadObjects{ submitMidi } {
    std::ifstream stream(CONFIG_FILE);
    auto config = nlohmann::json::parse(stream);
    auto midiClientName = static_cast<std::string>(config["midiClientName"]);
    initMidi(midiClientName);
    beginPollMidi();
}

AlsaMidiInterface::~AlsaMidiInterface() {
    endPollMidi();
}

inline static void AlsaErrorChecker(const int& error, const std::string& method) {
    if (error < 0) {
        throw std::runtime_error(method + " failed: " + static_cast<std::string>(snd_strerror(error)));
    }
}

void AlsaMidiInterface::initMidi(const std::string &clientName) {
    AlsaErrorChecker(snd_seq_open(&midiThreadObjects.sequencer, "default", SND_SEQ_OPEN_INPUT, SND_SEQ_NONBLOCK),
                     "snd_seq_open");
    AlsaErrorChecker(snd_seq_set_client_name(midiThreadObjects.sequencer, "Aeolus"), "snd_seq_set_client_name");
    AlsaErrorChecker(midiThreadObjects.portID = snd_seq_create_simple_port(
                             midiThreadObjects.sequencer, "Midi Listener",
                             SND_SEQ_PORT_CAP_WRITE | SND_SEQ_PORT_CAP_SUBS_WRITE,
                             SND_SEQ_PORT_TYPE_APPLICATION | SND_SEQ_PORT_TYPE_MIDI_GM |
                                     SND_SEQ_PORT_TYPE_MIDI_GENERIC | SND_SEQ_PORT_TYPE_SYNTHESIZER),
                     "snd_seq_create_simple_port");

    AlsaErrorChecker(snd_seq_connect_from(midiThreadObjects.sequencer, 0, getMidiClientId(clientName), 0),
                     "snd_seq_connect_from");

    midiThreadObjects.npfd = snd_seq_poll_descriptors_count(midiThreadObjects.sequencer, POLLIN);
    midiThreadObjects.pfd = std::make_unique<pollfd>();

    AlsaErrorChecker(snd_seq_nonblock(midiThreadObjects.sequencer, 1), "snd_seq_nonblock");
}

void AlsaMidiInterface::beginPollMidi() {
    midiThreadObjects.runningMidi = true;
    midiThread = std::thread(&midiHandler, &midiThreadObjects);
}

void *AlsaMidiInterface::midiHandler(const MidiThreadObjects *m) {
    do {
        snd_seq_event_t *event;
        if (const int err = snd_seq_event_input(m->sequencer, &event); err < 0 || !event) { continue; }
        if (MidiData midiData(*event); midiData.valid()) {
            try {
                m->submitMidiEvent(midiData);
            } catch (const std::exception &e) {
                std::cerr << "AlsaInterface::beginPollMidi - Exception thrown: " << e.what() << std::endl;
            } catch (...) {
                std::cerr << "AlsaInterface::beginPollMidi - Unknown exception in MIDI processing!" << std::endl;
            }
            snd_seq_free_event(event);
        }
        sched_yield();
    } while (m->runningMidi);
    return nullptr;
}

void AlsaMidiInterface::endPollMidi() {
    midiThreadObjects.runningMidi = false;
    if (midiThread.joinable()) { midiThread.join(); }
}

int AlsaMidiInterface::getMidiClientId(const std::string &clientName) const {
    auto clientStatus = 0;
    snd_seq_client_info_t *info = nullptr;
    snd_seq_client_info_alloca(&info);
    snd_seq_get_any_client_info(midiThreadObjects.sequencer, 0, info);
    do {
        const auto name = snd_seq_client_info_get_name(info);
        const auto id = snd_seq_client_info_get_client(info);
        if (std::strcmp(name, clientName.c_str()) == 0) { return id; }
        clientStatus = snd_seq_query_next_client(midiThreadObjects.sequencer, info);
    } while (clientStatus == 0);
    return -1;
}

#endif
