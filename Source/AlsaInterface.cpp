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
#include "aeolus/globals.h"
#include "aeolus/EngineGlobal.h"

#include <alsa/asoundlib.h>
#include <thread>
#include <nlohmann/json.hpp>
#include <fstream>

AlsaInterface::AlsaInterface() {
    const std::filesystem::path configFile = "./Resources/configs/audio.json";
    std::ifstream stream(configFile);
    auto config = nlohmann::json::parse(stream);
    midiClientName = config["midiClientName"];
    playbackDeviceName = config["playbackDeviceName"];

    initMidi();
    initAudio();

    beginPollMidi();
    beginPlayback();
}

AlsaInterface::~AlsaInterface() {
    endPollMidi();
    endPlayback();
}

int AlsaInterface::getMidiClientId() {
    int clientStatus = 0;
    snd_seq_client_info_t* info = nullptr;
    snd_seq_client_info_alloca(&info);
    snd_seq_get_any_client_info(sequencer, 0, info);
    do {
        const auto name = snd_seq_client_info_get_name(info);
        const auto id = snd_seq_client_info_get_client(info);
        if (std::strcmp(name, midiClientName.c_str()) == 0) {
            return id;
        }
        clientStatus = snd_seq_query_next_client(sequencer, info);
    } while (clientStatus == 0);
    return -1;
}

void AlsaInterface::initMidi() {
    if (snd_seq_open(&sequencer, "default", SND_SEQ_OPEN_INPUT, SND_SEQ_NONBLOCK) < 0) {
        throw std::runtime_error("Failed to open sequencer");
    }

    snd_seq_set_client_name(sequencer, "Aeolus");

    if ((portID = snd_seq_create_simple_port(sequencer, "Midi Listener",SND_SEQ_PORT_CAP_WRITE | SND_SEQ_PORT_CAP_SUBS_WRITE, SND_SEQ_PORT_TYPE_APPLICATION | SND_SEQ_PORT_TYPE_MIDI_GM | SND_SEQ_PORT_TYPE_MIDI_GENERIC | SND_SEQ_PORT_TYPE_SYNTHESIZER) < 0)) {
    	throw std::runtime_error("Failed to create MIDI port");
    }

	int err = snd_seq_connect_from(sequencer, 0, getMidiClientId(), 0);

	npfd = snd_seq_poll_descriptors_count(sequencer, POLLIN);
	pfd = std::make_unique<pollfd>();

	err = snd_seq_nonblock(sequencer, 1);
}

void AlsaInterface::beginPollMidi() {
    runningMidi = true;
	midiThread = std::make_unique<std::thread>([this] {
   		do {
   	 		// TODO: Poll doesn't work in debug for some reason.
			// snd_seq_poll_descriptors(sequencer,pfd.get(), npfd, POLLIN);
   			// if (poll(pfd.get(), npfd, -1) < 0) {
			//	continue;
   			// }
			snd_seq_event_t *event;
			if (const int err = snd_seq_event_input(sequencer, &event); err < 0) {
    		    continue;
    		}
   		    if (event && event->type & (SND_SEQ_EVENT_NOTEON | SND_SEQ_EVENT_NOTEOFF| SND_SEQ_EVENT_CONTROLLER | SND_SEQ_EVENT_PGMCHANGE)) {
   		            MidiData midiData;
				    midiData = *event;
    			    try {
    			        EngineGlobal::getInstance()->pushMidi(midiData);
    			    } catch (const std::exception& e) {
    			        std::cerr << "AlsaInterface::beginPollMidi - Exception thrown: " << e.what() << std::endl;
    			    } catch (...) {
    			        std::cerr << "AlsaInterface::beginPollMidi - Unknown exception in MIDI processing!" << std::endl;
    			    }
    			    snd_seq_free_event(event);
    			}
    	} while (runningMidi);
	});
}

void AlsaInterface::endPollMidi() {
    runningMidi = false;
    if (midiThread && midiThread->joinable()) {
        midiThread->join();
    }
}

void AlsaInterface::initAudio() {
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

    if (snd_pcm_hw_params_set_access(playback, hwParams, SND_PCM_ACCESS_MMAP_NONINTERLEAVED) < 0) {
        throw std::runtime_error("Failed to set access type");
    }

    if (snd_pcm_hw_params_set_format(playback, hwParams, SND_PCM_FORMAT_FLOAT) < 0) {
        throw std::runtime_error("Failed to set sample format");
    }

    auto sampleRate = static_cast<unsigned int>(SAMPLE_RATE);
    if (snd_pcm_hw_params_set_rate_near(playback, hwParams, &sampleRate, nullptr) < 0) {
        throw std::runtime_error("Failed to set sample rate");
    }

    assert(sampleRate == SAMPLE_RATE);

    if (snd_pcm_hw_params_set_channels(playback, hwParams, N_OUTPUT_CHANNELS) < 0) {
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
    audioThread = std::make_unique<std::thread>([this] {
        snd_pcm_uframes_t frames;
        snd_pcm_uframes_t offset;
        const snd_pcm_channel_area_t* areas;
        do {
            if (snd_pcm_wait(playback, 1000) < 0) {
                continue;
            }
            if ((frames = snd_pcm_avail_update(playback)) < 0 || frames < 1) {
                continue;
            }
            if (snd_pcm_mmap_begin(playback, &areas, &offset, &frames) < 0) {
                throw std::runtime_error("Failed to mmap audio buffer");
            }

            uint8_t* left_base = static_cast<uint8_t*>(areas[0].addr) + (areas[0].first >> 3);
            uint8_t* right_base = static_cast<uint8_t*>(areas[1].addr) + (areas[1].first >> 3);

            float* left = reinterpret_cast<float*>(left_base + offset * (areas[0].step >> 3));
            float* right = reinterpret_cast<float*>(right_base + offset * (areas[1].step >> 3));

            auto processedFrames = EngineGlobal::getInstance()->audioCallback(left, right, frames);
            // for (int i = 0; i < frames; i++) {
            //     left[i] = sin(2 * M_PI * 440 * ((float)i / SAMPLE_RATE));
            //     right[i] = sin(2 * M_PI * 440 * ((float)i / SAMPLE_RATE));
            // }

            if (auto committed = snd_pcm_mmap_commit(playback, offset, processedFrames); committed < 0) {
                std::cerr << "AlsaInterface::beginPlayback - snd_pcm_mmap_commit error: " << snd_strerror(committed) << std::endl;
                if (committed == -EPIPE) {
                    std::cerr << "Underrun detected, recovering...\n";
                    snd_pcm_prepare(playback);
                }
                continue;
            }
            snd_pcm_state_t state = snd_pcm_state(playback);

            if (state == SND_PCM_STATE_PREPARED) {
                if (int err = snd_pcm_start(playback); err < 0) {
                    std::cerr << "AlsaInterface::beginPlayback - snd_pcm_start error: " << snd_strerror(err) << std::endl;
                }
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