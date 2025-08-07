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
// #include <thread>
#include <pthread.h>
#include <sched.h>
#include <nlohmann/json.hpp>
#include <fstream>

AlsaInterface::AlsaInterface() {
    std::ifstream stream(CONFIG_FILE);
    auto config = nlohmann::json::parse(stream);
    midiClientName = config["midiClientName"];
    playbackDeviceName = "iec958:CARD=A,DEV=0";//config["playbackDeviceName"];

    initMidi();
    initAudio();

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
    AlsaErrorChecker(snd_seq_open(&sequencer, "default", SND_SEQ_OPEN_INPUT, SND_SEQ_NONBLOCK), "snd_seq_open");
    AlsaErrorChecker(snd_seq_set_client_name(sequencer, "Aeolus"), "snd_seq_set_client_name");
    AlsaErrorChecker(portID = snd_seq_create_simple_port(sequencer, "Midi Listener",
        SND_SEQ_PORT_CAP_WRITE | SND_SEQ_PORT_CAP_SUBS_WRITE, SND_SEQ_PORT_TYPE_APPLICATION |
        SND_SEQ_PORT_TYPE_MIDI_GM | SND_SEQ_PORT_TYPE_MIDI_GENERIC | SND_SEQ_PORT_TYPE_SYNTHESIZER), "snd_seq_create_simple_port");

	AlsaErrorChecker(snd_seq_connect_from(sequencer, 0, getMidiClientId(), 0), "snd_seq_connect_from");

	npfd = snd_seq_poll_descriptors_count(sequencer, POLLIN);
	pfd = std::make_unique<pollfd>();

	AlsaErrorChecker(snd_seq_nonblock(sequencer, 1), "snd_seq_nonblock");
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
    AlsaErrorChecker(snd_pcm_open(&threadState.playback, playbackDeviceName.c_str(), SND_PCM_STREAM_PLAYBACK, 0), "snd_pcm_open");

    snd_pcm_hw_params_t* hwParams{};
    AlsaErrorChecker(snd_pcm_hw_params_malloc(&hwParams), "snd_pcm_hw_params_malloc");
    AlsaErrorChecker(snd_pcm_hw_params_any(threadState.playback, hwParams), "snd_pcm_hw_params_any");

    snd_output_t *output;
    AlsaErrorChecker(snd_output_stdio_attach(&output, stdout, 0), "snd_output_stdio_attach");
    snd_pcm_hw_params_dump(hwParams, output);
    snd_output_close(output);

    AlsaErrorChecker(snd_pcm_hw_params_set_access(threadState.playback, hwParams, SND_PCM_ACCESS_RW_INTERLEAVED), "snd_pcm_hw_params_set_access");
    AlsaErrorChecker(snd_pcm_hw_params_set_format(threadState.playback, hwParams, static_cast<snd_pcm_format_t>(SAMPLE_FORMAT)), "snd_pcm_hw_params_set_format");
    auto sampleRate = static_cast<unsigned int>(SAMPLE_RATE);
    AlsaErrorChecker(snd_pcm_hw_params_set_rate_near(threadState.playback, hwParams, &sampleRate, nullptr), "snd_pcm_hw_params_set_rate_near");
    assert(sampleRate == SAMPLE_RATE);
    AlsaErrorChecker(snd_pcm_hw_params_set_channels(threadState.playback, hwParams, OUTPUT_CHANNELS), "snd_pcm_hw_params_set_channels");
    snd_pcm_uframes_t period = PERIOD_SIZE;
    auto periodDirection = 0;
    AlsaErrorChecker(snd_pcm_hw_params_set_period_size_near(threadState.playback, hwParams, &period, &periodDirection), "snd_pcm_hw_params_set_period_size_near");
    assert(period == PERIOD_SIZE);
    AlsaErrorChecker(snd_pcm_hw_params_set_buffer_size(threadState.playback, hwParams, NUMBER_FRAMES), "snd_pcm_hw_params_set_buffer_size");
    AlsaErrorChecker(snd_pcm_hw_params(threadState.playback, hwParams), "snd_pcm_hw_params");
    snd_pcm_hw_params_free(hwParams);

    snd_pcm_sw_params_t* swParams{};
    AlsaErrorChecker(snd_pcm_sw_params_malloc(&swParams), "snd_pcm_sw_params_malloc");
    AlsaErrorChecker(snd_pcm_sw_params_current(threadState.playback, swParams), "snd_pcm_sw_params_current");
    AlsaErrorChecker(snd_pcm_sw_params_set_start_threshold(threadState.playback, swParams, THRESHOLD_PCM), "snd_pcm_sw_params_set_start_threshold");
    AlsaErrorChecker(snd_pcm_sw_params(threadState.playback, swParams), "snd_pcm_sw_params");
    snd_pcm_sw_params_free(swParams);

    AlsaErrorChecker(snd_pcm_nonblock(threadState.playback, 1), "snd_pcm_nonblock");
    AlsaErrorChecker(snd_pcm_prepare(threadState.playback), "snd_pcm_prepare");
}

void AlsaInterface::beginPlayback() {
    threadState.runningAudio = true;
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

    if (ret = pthread_create(&tid, &attr, audioHandler, &threadState); ret < 0) {
        pthread_attr_destroy(&attr);
        throw std::runtime_error("pthread_create failed");
    }

    pthread_attr_destroy(&attr);
}

void* AlsaInterface::audioHandler(void *stateStruct) {
    MemoryUtilities::enableFlushToZero();
    const auto& ts = *static_cast<ThreadState*>(stateStruct);

    alignas(CACHE_LINE_SIZE) float fBuffer[NUMBER_SAMPLES];
    alignas(CACHE_LINE_SIZE) uint8_t oBuffer[NUMBER_SAMPLES * 3];

    // constexpr auto timeRequiredMicroseconds = NUMBER_FRAMES * SAMPLE_RATE_R * 1000000;

    // AlsaErrorChecker(snd_pcm_start(ts.playback), "snd_pcm_start");
    do {
        if (snd_pcm_wait(ts.playback, 1) < 0) {
            continue;
        }

        auto timer = std::chrono::high_resolution_clock::now();

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

// void* AlsaInterface::audioHandlerMmap(void *stateStruct) {
//     auto& ts = *static_cast<ThreadState*>(stateStruct);
//     MemoryUtilities::enableFlushToZero();
//     alignas(CACHE_LINE_SIZE) float fBuffer[NUMBER_SAMPLES];
//     constexpr auto timeRequiredMicroseconds = NUMBER_FRAMES * SAMPLE_RATE_R * 1000000;
//     do {
//         if (snd_pcm_wait(ts.playback, 1) < 0) {
//             continue;
//         }
//         if (ts.frames = snd_pcm_avail_update(ts.playback); ts.frames < NUMBER_FRAMES) {
//             if (snd_pcm_state(ts.playback) == SND_PCM_STATE_XRUN) {
//                 std::cerr << "Underrun detected, recovering..." << std::endl;
//                 AlsaErrorChecker(snd_pcm_prepare(ts.playback), "snd_pcm_prepare");
//             }
//             continue;
//         }
//         std::cerr << "Frames available, given " << ts.frames << " here we go..." << std::endl;
//
//         AlsaErrorChecker(snd_pcm_mmap_begin(ts.playback, &ts.areas, &ts.offset, &ts.frames), "snd_pcm_mmap_begin");
//
//         auto timer = std::chrono::high_resolution_clock::now();
//
//         // EngineGlobal::getInstance()->audioCallbackStereo(fBuffer);
//         for (int i = 0; i < NUMBER_FRAMES; ++i) {
//             auto time = static_cast<float>(i) * SAMPLE_RATE_R;
//             fBuffer[i*2 + 1] = fBuffer[i*2] = sinf(2.0f * std::numbers::pi_v<float> * 440.0f * time);
//         }
//
//         auto end = std::chrono::high_resolution_clock::now();
//
//         if constexpr (SAMPLE_RATE == kHz44100) {
//             auto data_ptr = static_cast<uint8_t*>(ts.areas[0].addr) + ts.offset * sizeof(int16_t) * OUTPUT_CHANNELS;
//             MemoryUtilities::ConvertF32toS16(fBuffer, reinterpret_cast<int16_t*>(data_ptr));
//         } else if constexpr (SAMPLE_RATE == kHz48000) {
//             auto data_ptr = static_cast<uint8_t*>(ts.areas[0].addr) + ts.offset * sizeof(uint8_t) * 3 * OUTPUT_CHANNELS;
//             MemoryUtilities::ConvertF32toS24(fBuffer, data_ptr);
//         } else {
//             throw std::runtime_error("Unsupported sample rate " + std::to_string(SAMPLE_RATE) + " for AlsaInterface::beginPlayback");
//         }
//
//         auto time = std::chrono::duration_cast<std::chrono::microseconds>(end - timer).count();
//         auto result = timeRequiredMicroseconds - time;
//         if (result > 0) {
//             std::cout << "Audio loop completed in " << time << " microseconds, or " << result << " microseconds ahead of time." << std::endl;
//         } else {
//             std::cerr << "Audio loop completed in " << time << " microseconds, or " << time - timeRequiredMicroseconds << " microseconds late." << std::endl;
//         }
//
//         if (auto committed = snd_pcm_mmap_commit(ts.playback, ts.offset, NUMBER_FRAMES); committed < 0) {
//             if (committed == -EPIPE) {
//                 std::cerr << "Underrun detected, recovering..." << std::endl;
//                 AlsaErrorChecker(snd_pcm_prepare(ts.playback), "snd_pcm_prepare");
//             } else if (committed == -ESTRPIPE) {
//                 std::cerr << "System suspended drivers, waiting for recovery...\n";
//                 while (snd_pcm_resume(ts.playback) == -EAGAIN) {
//                     sleep(1);
//                 }
//                 AlsaErrorChecker(snd_pcm_prepare(ts.playback), "snd_pcm_prepare");
//             } else {
//                 AlsaErrorChecker(static_cast<int>(committed), "snd_pcm_mmap_commit");
//             }
//         }
//         if (snd_pcm_state(ts.playback) == SND_PCM_STATE_PREPARED) {
//             std::cerr << "Underrun detected, recovering..." << std::endl;
//             AlsaErrorChecker(snd_pcm_start(ts.playback), "snd_pcm_start");
//         }
//     } while (ts.runningAudio);
//     std::cout << "Audio thread exiting" << std::endl;
//     MemoryUtilities::disableFlushToZero();
//     return nullptr;
// }

void AlsaInterface::endPlayback() {
    threadState.runningAudio = false;
    pthread_join(tid, nullptr);
    snd_pcm_drain(threadState.playback);
    snd_pcm_close(threadState.playback);
}
