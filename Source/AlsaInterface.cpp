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
#include <pthread.h>
#include <sched.h>
#include <nlohmann/json.hpp>
#include <fstream>

AlsaInterface::AlsaInterface() {
    std::ifstream stream(CONFIG_FILE);
    auto config = nlohmann::json::parse(stream);
    auto midiClientName = static_cast<std::string>(config["midiClientName"]);
    auto playbackDeviceName = static_cast<std::string>(config["playbackDeviceName"]);

    initMidi(midiClientName);
    initAudio(playbackDeviceName);

    beginPollMidi();
    beginPlayback();
}

AlsaInterface::~AlsaInterface() {
    endPollMidi();
    endPlayback();
}

inline static void AlsaErrorChecker(const int& error, const std::string& method) {
    if (error < 0) {
        throw std::runtime_error(method + " failed: " + static_cast<std::string>(snd_strerror(error)));
    }
}

 void AlsaInterface::createThread(pthread_t& tid, void* func, void* arg) {
    pthread_attr_t attr;
    int ret;

    if (ret = pthread_attr_init(&attr); ret < 0) {
        throw std::runtime_error("pthread_attr_init failed");
    }

    if (ret = pthread_attr_setschedpolicy(&attr, SCHED_FIFO); ret < 0) {
        pthread_attr_destroy(&attr);
        throw std::runtime_error("pthread_attr_setschedpolicy failed");
    }

    sched_param schedParam;
    schedParam.sched_priority = sched_get_priority_max(SCHED_FIFO);
    if (ret = pthread_attr_setschedparam(&attr, &schedParam); ret < 0) {
        pthread_attr_destroy(&attr);
        throw std::runtime_error("pthread_attr_setschedparam failed");
    }

    if (ret = pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED); ret < 0) {
        pthread_attr_destroy(&attr);
        throw std::runtime_error("pthread_attr_setinheritsched failed");
    }

    if (ret = pthread_create(&tid, &attr, reinterpret_cast<void *(*)(void *)>(func), &arg); ret < 0) {
        pthread_attr_destroy(&attr);
        throw std::runtime_error("pthread_create failed");
    }

    pthread_attr_destroy(&attr);
}

int AlsaInterface::getMidiClientId(const std::string &clientName) {
    int clientStatus = 0;
    snd_seq_client_info_t* info = nullptr;
    snd_seq_client_info_alloca(&info);
    snd_seq_get_any_client_info(midiThreadObjects.sequencer, 0, info);
    do {
        const auto name = snd_seq_client_info_get_name(info);
        const auto id = snd_seq_client_info_get_client(info);
        if (std::strcmp(name, clientName.c_str()) == 0) {
            return id;
        }
        clientStatus = snd_seq_query_next_client(midiThreadObjects.sequencer, info);
    } while (clientStatus == 0);
    return -1;
}

void AlsaInterface::initMidi(const std::string &clientName) {
    AlsaErrorChecker(snd_seq_open(&midiThreadObjects.sequencer, "default", SND_SEQ_OPEN_INPUT, SND_SEQ_NONBLOCK), "snd_seq_open");
    AlsaErrorChecker(snd_seq_set_client_name(midiThreadObjects.sequencer, "Aeolus"), "snd_seq_set_client_name");
    AlsaErrorChecker(midiThreadObjects.portID = snd_seq_create_simple_port(midiThreadObjects.sequencer, "Midi Listener",
        SND_SEQ_PORT_CAP_WRITE | SND_SEQ_PORT_CAP_SUBS_WRITE, SND_SEQ_PORT_TYPE_APPLICATION |
        SND_SEQ_PORT_TYPE_MIDI_GM | SND_SEQ_PORT_TYPE_MIDI_GENERIC | SND_SEQ_PORT_TYPE_SYNTHESIZER), "snd_seq_create_simple_port");

	AlsaErrorChecker(snd_seq_connect_from(midiThreadObjects.sequencer, 0, getMidiClientId(clientName), 0), "snd_seq_connect_from");

	midiThreadObjects.npfd = snd_seq_poll_descriptors_count(midiThreadObjects.sequencer, POLLIN);
	midiThreadObjects.pfd = std::make_unique<pollfd>();

	AlsaErrorChecker(snd_seq_nonblock(midiThreadObjects.sequencer, 1), "snd_seq_nonblock");
}

void AlsaInterface::beginPollMidi() {
    midiThreadObjects.runningMidi = true;
    createThread(midiTID, reinterpret_cast<void*>(&midiHandler), &midiThreadObjects);
}

void* AlsaInterface::midiHandler(void *stateStruct) {
    const auto& ms = *static_cast<MidiThreadObjects*>(stateStruct);
   	do {
   		// TODO: Poll doesn't work in debug for some reason.
		 snd_seq_poll_descriptors(ms.sequencer,ms.pfd.get(), ms.npfd, POLLIN);
   		 if (poll(ms.pfd.get(), ms.npfd, -1) < 0) {
			continue;
   		 }
		snd_seq_event_t *event;
		if (const int err = snd_seq_event_input(ms.sequencer, &event); err < 0 || !event) {
    	    continue;
    	}
   	    if (MidiData midiData(*event); midiData.valid()) {
   	        try {
   	            EngineGlobal::getInstance()->pushMidi(midiData);
   	        } catch (const std::exception& e) {
   	            std::cerr << "AlsaInterface::beginPollMidi - Exception thrown: " << e.what() << std::endl;
   	        } catch (...) {
   	            std::cerr << "AlsaInterface::beginPollMidi - Unknown exception in MIDI processing!" << std::endl;
   	        }
   	        snd_seq_free_event(event);
   	    }
    } while (ms.runningMidi);
}

void AlsaInterface::endPollMidi() {
    midiThreadObjects.runningMidi = false;
    if (midiTID) {
        pthread_join(midiTID, nullptr);
    }
}

void AlsaInterface::initAudio(const std::string &deviceName) {
    AlsaErrorChecker(snd_pcm_open(&audioThreadObjects.playback, deviceName.c_str(), SND_PCM_STREAM_PLAYBACK, 0), "snd_pcm_open");

    snd_pcm_hw_params_t* hwParams{};
    AlsaErrorChecker(snd_pcm_hw_params_malloc(&hwParams), "snd_pcm_hw_params_malloc");
    AlsaErrorChecker(snd_pcm_hw_params_any(audioThreadObjects.playback, hwParams), "snd_pcm_hw_params_any");

    snd_output_t *output;
    AlsaErrorChecker(snd_output_stdio_attach(&output, stdout, 0), "snd_output_stdio_attach");
    snd_pcm_hw_params_dump(hwParams, output);
    snd_output_close(output);

    AlsaErrorChecker(snd_pcm_hw_params_set_access(audioThreadObjects.playback, hwParams, SND_PCM_ACCESS_RW_INTERLEAVED), "snd_pcm_hw_params_set_access");
    AlsaErrorChecker(snd_pcm_hw_params_set_format(audioThreadObjects.playback, hwParams, static_cast<snd_pcm_format_t>(SAMPLE_FORMAT)), "snd_pcm_hw_params_set_format");
    auto sampleRate = static_cast<unsigned int>(SAMPLE_RATE);
    AlsaErrorChecker(snd_pcm_hw_params_set_rate_near(audioThreadObjects.playback, hwParams, &sampleRate, nullptr), "snd_pcm_hw_params_set_rate_near");
    assert(sampleRate == SAMPLE_RATE);
    AlsaErrorChecker(snd_pcm_hw_params_set_channels(audioThreadObjects.playback, hwParams, OUTPUT_CHANNELS), "snd_pcm_hw_params_set_channels");
    snd_pcm_uframes_t period = PERIOD_SIZE;
    auto periodDirection = 0;
    AlsaErrorChecker(snd_pcm_hw_params_set_period_size_near(audioThreadObjects.playback, hwParams, &period, &periodDirection), "snd_pcm_hw_params_set_period_size_near");
    assert(period == PERIOD_SIZE);
    AlsaErrorChecker(snd_pcm_hw_params_set_buffer_size(audioThreadObjects.playback, hwParams, NUMBER_FRAMES), "snd_pcm_hw_params_set_buffer_size");
    AlsaErrorChecker(snd_pcm_hw_params(audioThreadObjects.playback, hwParams), "snd_pcm_hw_params");
    snd_pcm_hw_params_free(hwParams);

    snd_pcm_sw_params_t* swParams{};
    AlsaErrorChecker(snd_pcm_sw_params_malloc(&swParams), "snd_pcm_sw_params_malloc");
    AlsaErrorChecker(snd_pcm_sw_params_current(audioThreadObjects.playback, swParams), "snd_pcm_sw_params_current");
    AlsaErrorChecker(snd_pcm_sw_params_set_start_threshold(audioThreadObjects.playback, swParams, THRESHOLD_PCM), "snd_pcm_sw_params_set_start_threshold");
    AlsaErrorChecker(snd_pcm_sw_params(audioThreadObjects.playback, swParams), "snd_pcm_sw_params");
    snd_pcm_sw_params_free(swParams);

    AlsaErrorChecker(snd_pcm_nonblock(audioThreadObjects.playback, 1), "snd_pcm_nonblock");
    AlsaErrorChecker(snd_pcm_prepare(audioThreadObjects.playback), "snd_pcm_prepare");
}

void AlsaInterface::beginPlayback() {
    audioThreadObjects.runningAudio = true;
    createThread(audioTID, reinterpret_cast<void*>(&audioHandler), &audioThreadObjects);
}

void* AlsaInterface::audioHandler(void *stateStruct) {
    MemoryUtilities::enableFlushToZero();
    const auto& ts = *static_cast<AudioThreadObjects*>(stateStruct);

    alignas(CACHE_LINE_SIZE) float fBuffer[NUMBER_SAMPLES];
    alignas(CACHE_LINE_SIZE) uint8_t oBuffer[NUMBER_SAMPLES * 3];

    // constexpr auto timeRequiredMicroseconds = NUMBER_FRAMES * SAMPLE_RATE_R * 1000000;

    // AlsaErrorChecker(snd_pcm_start(ts.playback), "snd_pcm_start");
    do {
        if (snd_pcm_wait(ts.playback, 1) < 0) {
            continue;
        }
        // auto timer = std::chrono::high_resolution_clock::now();
        // EngineGlobal::getInstance()->audioCallbackStereo(fBuffer);
        for (int i = 0; i < NUMBER_FRAMES; ++i) {
            auto time = static_cast<float>(i) * SAMPLE_RATE_R;
            fBuffer[i*2 + 1] = fBuffer[i*2] = sinf(2.0f * std::numbers::pi_v<float> * 440.0f * time);
        }
        MemoryUtilities::ConvertF32toS24(fBuffer, oBuffer);
        snd_pcm_writei(ts.playback, oBuffer, NUMBER_FRAMES);
        // auto end = std::chrono::high_resolution_clock::now();
        // auto time = std::chrono::duration_cast<std::chrono::microseconds>(end - timer).count();
        // auto result = timeRequiredMicroseconds - time;
        // if (result > 0) {
        //     std::cout << "Audio loop completed in " << time << " microseconds, or " << result << " microseconds ahead of time." << std::endl;
        // } else {
        //     std::cerr << "Audio loop completed in " << time << " microseconds, or " << time - timeRequiredMicroseconds << " microseconds late." << std::endl;
        // }
    } while (ts.runningAudio);
    MemoryUtilities::disableFlushToZero();
    return nullptr;
}

void AlsaInterface::endPlayback() {
    audioThreadObjects.runningAudio = false;
    if (audioTID) {
        pthread_join(audioTID, nullptr);
    }
    snd_pcm_drain(audioThreadObjects.playback);
    snd_pcm_close(audioThreadObjects.playback);
}
