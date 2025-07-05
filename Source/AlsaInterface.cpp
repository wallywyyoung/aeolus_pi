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

#include "AlsaInterface.h"

#include <fstream>

#include "aeolus/EngineGlobal.h"

#include <alsa/asoundlib.h>
#include <vector>
#include <thread>
#include <nlohmann/json.hpp>

AlsaInterface::AlsaInterface()  {
    const std::filesystem::path configFile = "./Resources/configs/audio.json";
    const nlohmann::json config = nlohmann::json::parse(std::ifstream(configFile));
    midiClientName = config["midiClientName"];
    playbackDeviceName = config["playbackDeviceName"];

    initMidi();
    initAudio(N_OUTPUT_CHANNELS, SAMPLE_RATE, BPS_RATE);

    beginPollMidi();
    beginPlayback();
}

AlsaInterface::~AlsaInterface() {
    endPollMidi();
    endPlayback();
}

int AlsaInterface::getMidiClientId() const {
    int clientStatus = 0, index = 0;
    while (clientStatus >= 0) {
        snd_seq_client_info_t* info = nullptr;
        const int id = snd_seq_client_info_get_client(info);
        if (char const* name = snd_seq_client_info_get_name(info); name == midiClientName.c_str()) {
            return id;
        }
        ++index;
        clientStatus = snd_seq_query_next_client(sequencer, info);
    }
    return -1;
}

void AlsaInterface::initMidi() {
    if (snd_seq_open(&sequencer, "default", SND_SEQ_OPEN_INPUT, 0) < 0) {
        throw std::runtime_error("Failed to open sequencer");
    }

    snd_seq_set_client_name(sequencer, "Aeolus");

    if ((portID = snd_seq_create_simple_port(sequencer, "Midi Listener",SND_SEQ_PORT_CAP_WRITE | SND_SEQ_PORT_CAP_SUBS_WRITE, SND_SEQ_PORT_TYPE_APPLICATION) < 0)) {
        throw std::runtime_error("Failed to create MIDI port");
    }

    {
        snd_seq_addr_t sender, dest;
        snd_seq_port_subscribe_t *subs;
        sender.client = snd_seq_client_id(sequencer);
        sender.port = 0;
        dest.client = getMidiClientId();
        dest.port = portID;
        snd_seq_port_subscribe_alloca(&subs);
        snd_seq_port_subscribe_set_sender(subs, &sender);
        snd_seq_port_subscribe_set_dest(subs, &dest);
        snd_seq_port_subscribe_set_queue(subs, 1);
        snd_seq_port_subscribe_set_time_update(subs, 1);
        snd_seq_port_subscribe_set_time_real(subs, 1);
        if (snd_seq_subscribe_port(sequencer, subs) < 0) {
            throw std::runtime_error("Failed to subscribe to MIDI port");
        }
    }

    npfd = snd_seq_poll_descriptors_count(sequencer, POLLIN);
    pfd = static_cast<pollfd *>(malloc(sizeof(pollfd) * npfd));
    snd_seq_poll_descriptors(sequencer,pfd, npfd, POLLIN);
}

void AlsaInterface::beginPollMidi() {
    runningMidi = true;
    midiThread = std::make_unique<std::thread>([this]() {
    auto transferBuffer = std::vector<MidiData>();
    do {
        if (::poll(pfd, npfd, 100000) <= 0) {
            continue;
        }
        snd_seq_event_t *event;
        int bytes = 0;
        transferBuffer.clear();
        bool next = false;
        while (next || snd_seq_event_input_pending(sequencer, 1) > 0) {
            bytes = snd_seq_event_input(sequencer, &event);
            next = bytes > 0;
            auto data = MidiData(*event);
            if (data.eventType == MidiData::IGNORE) {
                continue;
            }
            printf( std::to_string(data.channel).c_str());
            transferBuffer.push_back(data);
        };
        EngineGlobal::getInstance().midiBuffer.push(transferBuffer);
    } while (runningMidi);
    });
}

void AlsaInterface::endPollMidi() {
    runningMidi = false;
    if (midiThread && midiThread->joinable()) {
        midiThread->join();
    }

}

void AlsaInterface::initAudio(const int channels, unsigned int sampleRate, size_t bufferSize) {
    snd_pcm_hw_params_t* hwParams{};

    if (snd_pcm_open(&playback, playbackDeviceName.c_str(), SND_PCM_STREAM_PLAYBACK, 0) < 0) {
        throw std::runtime_error("Failed to open playback device");
    }
    if (snd_pcm_hw_params_malloc(&hwParams) < 0) {
        throw std::runtime_error("Failed to allocate hardware parameters");
    }
    if (snd_pcm_hw_params_any(playback, hwParams) < 0) {
        throw std::runtime_error("Failed to initialize hardware parameters");
    }
    if (snd_pcm_hw_params_set_access(playback, hwParams, SND_PCM_ACCESS_RW_NONINTERLEAVED) < 0) {
        throw std::runtime_error("Failed to set access type");
    }
    if (snd_pcm_hw_params_set_format(playback, hwParams, SND_PCM_FORMAT_FLOAT) < 0) {
        throw std::runtime_error("Failed to set sample format");
    }
    if (snd_pcm_hw_params_set_rate_near(playback, hwParams, &sampleRate, nullptr) < 0) {
        throw std::runtime_error("Failed to set sample rate");
    }
    if (snd_pcm_hw_params_set_channels(playback, hwParams, channels) < 0) {
        throw std::runtime_error("Failed to set number of channels");
    }
    if (snd_pcm_hw_params(playback, hwParams) < 0) {
        throw std::runtime_error("Failed to set hardware parameters");
    }

    snd_pcm_hw_params_free(hwParams);

    if (snd_pcm_prepare(playback) < 0) {
        throw std::runtime_error("Failed to prepare playback device");
    }
}

void AlsaInterface::beginPlayback() {
    runningAudio = true;
    audioThread = std::make_unique<std::thread>([this]() {
        snd_pcm_uframes_t frames;
        do {
            if (snd_pcm_wait(playback, 1000) < 0) {
                break;
            }
            if ((frames = snd_pcm_avail_update(playback)) < 0) {
                throw std::runtime_error("Failed to update available frames");
            }
            float buffer[4096];
            EngineGlobal::getInstance().audioCallback(&buffer[0], &buffer[2048], 4096);
            if (snd_pcm_writei(playback, buffer, frames) < 0) {
                break;
            }
        } while (runningAudio);
    });
}

void AlsaInterface::endPlayback() {
    runningAudio = false;

    if (audioThread && audioThread->joinable()) {
        audioThread->join();
    }

    snd_pcm_drain(playback);
    snd_pcm_close(playback);
}