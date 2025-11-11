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

#include "AlsaAudioInterface.h"
#include <alsa/asoundlib.h>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include "aeolus/utilities/SimdUtilities.h"

using namespace std::literals::chrono_literals;

AlsaAudioInterface::AlsaAudioInterface(std::function<void(float (&out)[PROCESS_SAMPLES_SIZE])> processAudio) : audioThreadObjects{ processAudio } {
    std::ifstream stream(CONFIG_FILE);
    auto config = nlohmann::json::parse(stream);
    auto playbackDeviceName = static_cast<std::string>(config["playbackDeviceName"]);
    initAudio(playbackDeviceName);
    beginPlayback();
}

AlsaAudioInterface::~AlsaAudioInterface() { endPlayback(); }

inline static void AlsaErrorChecker(const int& error, const std::string& method) {
    if (error < 0) {
        throw std::runtime_error(method + " failed: " + static_cast<std::string>(snd_strerror(error)));
    }
}

void AlsaAudioInterface::initAudio(const std::string &deviceName) {
    AlsaErrorChecker(snd_pcm_open(&audioThreadObjects.playback, deviceName.c_str(), SND_PCM_STREAM_PLAYBACK, 0), "snd_pcm_open");

    snd_pcm_hw_params_t* hwParams{};
    AlsaErrorChecker(snd_pcm_hw_params_malloc(&hwParams), "snd_pcm_hw_params_malloc");
    AlsaErrorChecker(snd_pcm_hw_params_any(audioThreadObjects.playback, hwParams), "snd_pcm_hw_params_any");

#if DEBUG
    snd_output_t *output;
    AlsaErrorChecker(snd_output_stdio_attach(&output, stdout, 0), "snd_output_stdio_attach");
    snd_pcm_hw_params_dump(hwParams, output);
    snd_output_close(output);
#endif

    AlsaErrorChecker(snd_pcm_hw_params_set_access(audioThreadObjects.playback, hwParams, SND_PCM_ACCESS_RW_INTERLEAVED), "snd_pcm_hw_params_set_access");
    AlsaErrorChecker(snd_pcm_hw_params_set_format(audioThreadObjects.playback, hwParams, static_cast<snd_pcm_format_t>(SAMPLE_FORMAT)), "snd_pcm_hw_params_set_format");
    auto sampleRate = static_cast<unsigned int>(SAMPLE_RATE);
    AlsaErrorChecker(snd_pcm_hw_params_set_rate_near(audioThreadObjects.playback, hwParams, &sampleRate, nullptr), "snd_pcm_hw_params_set_rate_near");
    assert(sampleRate == SAMPLE_RATE);
    AlsaErrorChecker(snd_pcm_hw_params_set_channels(audioThreadObjects.playback, hwParams, OUTPUT_CHANNELS), "snd_pcm_hw_params_set_channels");
    snd_pcm_uframes_t period = ALSA_PERIOD_SIZE;
    auto periodDirection = 0;
    AlsaErrorChecker(snd_pcm_hw_params_set_period_size_near(audioThreadObjects.playback, hwParams, &period, &periodDirection), "snd_pcm_hw_params_set_period_size_near");
    assert(period == ALSA_PERIOD_SIZE);
    AlsaErrorChecker(snd_pcm_hw_params_set_buffer_size(audioThreadObjects.playback, hwParams, ALSA_BUFFER_FRAMES_SIZE), "snd_pcm_hw_params_set_buffer_size");
    AlsaErrorChecker(snd_pcm_hw_params(audioThreadObjects.playback, hwParams), "snd_pcm_hw_params");
    snd_pcm_hw_params_free(hwParams);

    snd_pcm_sw_params_t* swParams{};
    AlsaErrorChecker(snd_pcm_sw_params_malloc(&swParams), "snd_pcm_sw_params_malloc");
    AlsaErrorChecker(snd_pcm_sw_params_current(audioThreadObjects.playback, swParams), "snd_pcm_sw_params_current");
    AlsaErrorChecker(snd_pcm_sw_params_set_start_threshold(audioThreadObjects.playback, swParams, ALSA_THRESHOLD), "snd_pcm_sw_params_set_start_threshold");
    AlsaErrorChecker(snd_pcm_sw_params_set_avail_min(audioThreadObjects.playback, swParams, ALSA_MINIMUM_FRAMES), "snd_pcm_sw_params_set_avail_min");
    AlsaErrorChecker(snd_pcm_sw_params(audioThreadObjects.playback, swParams), "snd_pcm_sw_params");
    snd_pcm_sw_params_free(swParams);

    AlsaErrorChecker(snd_pcm_nonblock(audioThreadObjects.playback, 1), "snd_pcm_nonblock");
    AlsaErrorChecker(snd_pcm_prepare(audioThreadObjects.playback), "snd_pcm_prepare");

    std::array<uint8_t, ALSA_PERIOD_SIZE * 3 * OUTPUT_CHANNELS> buffer;
    buffer.fill(0);
    snd_pcm_writei(audioThreadObjects.playback, &buffer[0], ALSA_PERIOD_SIZE);
}

void AlsaAudioInterface::beginPlayback() {
    audioThreadObjects.runningAudio = true;
    audioThread = std::thread(&audioHandler, &audioThreadObjects);
}

void AlsaAudioInterface::audioHandler(const AudioThreadObjects * a) {
    SimdUtilities::enableFlushToZero();
    snd_pcm_sframes_t available, written;
    alignas(CACHE_LINE_SIZE) static float fBuffer[PROCESS_SAMPLES_SIZE];
    alignas(CACHE_LINE_SIZE) static uint8_t oBuffer[PROCESS_SAMPLES_SIZE * 3];
    do {
        available = snd_pcm_avail_update(a->playback);
        if (available < 0) {
            std::cerr << "snd_pcm_avail_update error: " << snd_strerror(available) << std::endl;
            AlsaErrorChecker(snd_pcm_recover(a->playback, available, 1), "snd_pcm_recover");
            continue;
        }
        if (available < PROCESS_FRAMES_SIZE) {
            std::this_thread::sleep_for(100us);
            continue;
        }

        a->processAudio(fBuffer);
        SimdUtilities::convertF32ToS24(fBuffer, oBuffer);
        written = snd_pcm_writei(a->playback, oBuffer, PROCESS_FRAMES_SIZE);

        if (written < 0) {
            std::cerr << "snd_pcm_writei failed: " << snd_strerror(written) << std::endl;
            AlsaErrorChecker(snd_pcm_recover(a->playback, written, 1), "snd_pcm_recover");
            continue;
        }

        if (written != PROCESS_FRAMES_SIZE) {
            std::cerr << "Partial number of frames written." << std::endl;
        }
    } while (a->runningAudio);
    SimdUtilities::disableFlushToZero();
}

void AlsaAudioInterface::endPlayback() {
    audioThreadObjects.runningAudio = false;
    if (audioThread.joinable()) {
        audioThread.join();
    }
    snd_pcm_drain(audioThreadObjects.playback);
    snd_pcm_close(audioThreadObjects.playback);
}

#endif
