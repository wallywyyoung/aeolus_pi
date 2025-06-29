//
// Created by Wally Young on 6/6/25.
//

#pragma once

#include "AudioServer.h"
#include "MidiData.h"
#include "ObjectBuffer.h"

#include <asoundlib.h>


class AlsaInterface : AudioServer {
private:
    void initMidi();
    void initAudio(int channels, unsigned int sampleRate, size_t bufferSize);
    int playbackThread(snd_pcm_sframes_t numberFrames);
//    Midi Params
    snd_seq_t* sequencer;
    int portID;
    int npfd;
    struct pollfd* pfd;
//    Audio Params
    snd_pcm_t* playback;
    bool runningAudio, runningMidi;
    ObjectBuffer<MidiData> midiBuffer;
public:
    AlsaInterface(float sampleRate);
    void beginPollMidi();
    void endPollMidi();
    void beginPlayback();
    void endPlayback();
    ~AlsaInterface();
};
