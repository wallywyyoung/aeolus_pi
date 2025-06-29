//
// Created by Wally Young on 6/6/25.
//

#include "AlsaInterface.h"
#include "aeolus/globals.h"

#include <asoundlib.h>
#include <stdexcept>
#include <cassert>
#include <vector>

AlsaInterface::AlsaInterface()  {
    initMidi();
    initAudio(N_OUTPUT_CHANNELS, SAMPLE_RATE, BPS_RATE);
}

void AlsaInterface::beginPollMidi() {
    runningMidi = true;
    std::thread thisThread([this]() {
        auto transferBuffer = std::vector<MidiData>();
        do {
            if (::poll(pfd, npfd, 100000) <= 0) {
                return;
            }
            snd_seq_event_t *event;
            transferBuffer.clear();
            do {
                snd_seq_event_input(sequencer, &event);
                auto data = MidiData(*event);
                if (data.eventType == MidiData::IGNORE) {
                    continue;
                }
                transferBuffer.push_back(data);
            } while (snd_seq_event_input_pending(sequencer, 0) > 0);
            midiBuffer.push(transferBuffer);
        } while (runningMidi);
    });
}

void AlsaInterface::endPollMidi() {
    runningMidi = false;
}

AlsaInterface::~AlsaInterface() {
    // Midi
    snd_seq_close(sequencer);
    free(pfd);

    // Audio
    snd_pcm_drain(playback);
    snd_pcm_close(playback);
}

void AlsaInterface::initMidi() {
    snd_seq_open(&sequencer, "default", SND_SEQ_OPEN_INPUT, 0);
    snd_seq_set_client_name(sequencer, "Aeolus");
    portID = snd_seq_create_simple_port(sequencer, "Midi Listener",SND_SEQ_PORT_CAP_WRITE | SND_SEQ_PORT_CAP_SUBS_WRITE, SND_SEQ_PORT_TYPE_APPLICATION);
    npfd = snd_seq_poll_descriptors_count(sequencer, POLLIN);
    pfd = static_cast<pollfd *>(malloc(sizeof(pollfd) * npfd));
    snd_seq_poll_descriptors(sequencer,pfd, npfd, POLLIN);
}

void AlsaInterface::initAudio(int channels, unsigned int sampleRate, size_t bufferSize) {
    snd_pcm_format_t format = SND_PCM_FORMAT_FLOAT;
    snd_pcm_hw_params_t* hwParams;

    assert(0 > snd_pcm_hw_params_malloc(&hwParams));
    assert(0 > snd_pcm_open(&playback, "default", SND_PCM_STREAM_PLAYBACK, 0));
    assert(0 > snd_pcm_hw_params_any(playback, hwParams));
    assert(0 > snd_pcm_hw_params_set_access(playback, hwParams, SND_PCM_ACCESS_RW_NONINTERLEAVED));
    assert(0 > snd_pcm_hw_params_set_format(playback, hwParams, format));
    assert(0 > snd_pcm_hw_params_set_rate_near(playback, hwParams, &sampleRate, 0));
    assert(0 > snd_pcm_hw_params_set_channels(playback, hwParams, channels));
    assert(0 > snd_pcm_hw_params(playback, hwParams));

    snd_pcm_hw_params_free(hwParams);

    assert(snd_pcm_prepare(playback));
}

void AlsaInterface::beginPlayback() {
    runningAudio = true;
    std::thread thisThread([this]() {
        snd_pcm_sframes_t frames;
        do {
            if (snd_pcm_wait(playback, 1000) < 0) {
                break;
            }
            if ((frames = snd_pcm_avail_update(playback)) < 0) {
                if (frames == -EPIPE) {
                    break;
                } else {
                    break;
                }
            }
            char buffer[4096];
            if (snd_pcm_writei(playback, buffer, frames) < 0) {
                break;
            }
        } while (runningAudio);
    });
}

void AlsaInterface::endPlayback() {
    runningAudio = false;
}