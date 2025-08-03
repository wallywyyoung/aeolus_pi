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
#include "MemoryUtilities.h"

#include <alsa/asoundlib.h>
#include <thread>
#include <nlohmann/json.hpp>
#include <fstream>

AlsaInterface::AlsaInterface() {
    std::ifstream stream(CONFIG_FILE);
    auto config = nlohmann::json::parse(stream);
    midiClientName = config["midiClientName"];
    playbackDeviceName = "hw:0,0";//config["playbackDeviceName"];

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
    int err = 0;
    std::string errStr;

    if (err = snd_pcm_open(&playback, playbackDeviceName.c_str(), SND_PCM_STREAM_PLAYBACK, 0); err < 0) {
        errStr = "snd_pcm_open failed: ";
        errStr.append(snd_strerror(err));
        throw std::runtime_error(errStr);
    }

    if (err = snd_pcm_hw_params_malloc(&hwParams); err < 0) {
        errStr = "snd_pcm_hw_params_malloc failed: ";
        errStr.append(snd_strerror(err));
        throw std::runtime_error(errStr);
    }

    if (err = snd_pcm_hw_params_any(playback, hwParams); err < 0) {
        errStr = "snd_pcm_hw_params_any failed: ";
        errStr.append(snd_strerror(err));
        throw std::runtime_error(errStr);
    }

    if (err = snd_pcm_hw_params_set_access(playback, hwParams, SND_PCM_ACCESS_MMAP_INTERLEAVED); err < 0) {
        errStr = "snd_pcm_hw_params_set_access failed: ";
        errStr.append(snd_strerror(err));
        throw std::runtime_error(errStr);
    }

    if (err = snd_pcm_hw_params_set_format(playback, hwParams, SND_PCM_FORMAT_S16); err < 0) {
        errStr = "snd_pcm_hw_params_set_format failed: ";
        errStr.append(snd_strerror(err));
        throw std::runtime_error(errStr);
    }

    auto sampleRate = static_cast<unsigned int>(SAMPLE_RATE);
    if (err = snd_pcm_hw_params_set_rate_near(playback, hwParams, &sampleRate, nullptr); err < 0) {
        errStr = "snd_pcm_hw_params_set_rate_near failed: ";
        errStr.append(snd_strerror(err));
        throw std::runtime_error(errStr);
    }

    assert(sampleRate == SAMPLE_RATE);

    if (err = snd_pcm_hw_params_set_channels(playback, hwParams, N_OUTPUT_CHANNELS); err < 0) {
        errStr = "snd_pcm_hw_params_set_channels failed: ";
        errStr.append(snd_strerror(err));
        throw std::runtime_error(errStr);
    }
    if (err = snd_pcm_hw_params(playback, hwParams); err < 0) {
        errStr = "snd_pcm_hw_params failed: ";
        errStr.append(snd_strerror(err));
        throw std::runtime_error(errStr);
    }

    snd_pcm_hw_params_free(hwParams);

    if (err = snd_pcm_prepare(playback); err < 0) {
        errStr = "snd_pcm_prepare failed: ";
        errStr.append(snd_strerror(err));
        throw std::runtime_error(errStr);
    }
}

void AlsaInterface::beginPlayback() {
    runningAudio = true;
    audioThread = std::make_unique<std::thread>([this] {
        MemoryUtilities::enableFlushToZero();
        int err;
        std::string errStr;
        snd_pcm_uframes_t frames;
        snd_pcm_uframes_t offset;
        const snd_pcm_channel_area_t* areas;
        // alignas(MemoryUtilities::CACHE_LINE_SIZE) int16_t iBuffer[MemoryUtilities::AUDIO_BUFFER_SIZE*N_OUTPUT_CHANNELS];
        alignas(MemoryUtilities::CACHE_LINE_SIZE) float fBuffer[MemoryUtilities::AUDIO_BUFFER_SIZE];
        do {
            if (snd_pcm_wait(playback, 1000) < 0) {
                continue;
            }
            if (frames = snd_pcm_avail_update(playback); frames < MemoryUtilities::AUDIO_CHANNEL_BUFFER_SIZE) {
                continue;
            }
            if (err = snd_pcm_mmap_begin(playback, &areas, &offset, &frames); err < 0) {
                errStr = "snd_pcm_mmap_begin failed: ";
                errStr.append(snd_strerror(err));
                throw std::runtime_error(errStr);
            }
            auto data_ptr = reinterpret_cast<int16_t*>(static_cast<char*>(areas[0].addr) + offset * sizeof(int16_t) * N_OUTPUT_CHANNELS);
            EngineGlobal::getInstance()->audioCallbackStereo(fBuffer);
            MemoryUtilities::ConvertF32toS16(fBuffer, &data_ptr[0]);
            // for (int i = 0; i < MemoryUtilities::AUDIO_BUFFER_SIZE; ++i) {
            //     data_ptr[i] = std::round(32767.0f * std::clamp(fBuffer[i], -1.0f, 1.0f));
            //     // std::cout << fBuffer[i] << " in loop " << data_ptr[i] << " vs in neon " << iBuffer[i] << std::endl;
            // }

            if (auto committed = snd_pcm_mmap_commit(playback, offset, MemoryUtilities::AUDIO_CHANNEL_BUFFER_SIZE); committed < 0) {
                if (committed == -EPIPE) {
                    std::cerr << "Underrun detected, recovering...\n";
                    snd_pcm_prepare(playback);
                } else {
                    errStr = "snd_pcm_mmap_commit failed: ";
                    errStr.append(snd_strerror(err));
                    throw std::runtime_error(errStr);
                }
                continue;
            }
            snd_pcm_state_t state = snd_pcm_state(playback);

            if (state == SND_PCM_STATE_PREPARED) {
                if (err = snd_pcm_start(playback); err < 0) {
                    errStr = "snd_pcm_start failed: ";
                    errStr.append(snd_strerror(err));
                    throw std::runtime_error(errStr);
                }
            }
        } while (runningAudio);
        MemoryUtilities::disableFlushToZero();
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