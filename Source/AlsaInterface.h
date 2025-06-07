//
// Created by Wally Young on 6/6/25.
//

#pragma once

class AlsaInterface {
private:
    void initMidi();
    void initAudio(int channels, unsigned int sampleRate, int bufferSize);
//    Midi Params
    struct snd_seq_t* sequencer;
    int portID;
    int npfd;
    struct pollfd* pfd;
//    Audio Params
    struct snd_pcm_t* playback;
public:
    AlsaInterface(int channels = 2, unsigned int sampleRate = 441000, int bufferSize = 1024);
    void poll();
    ~AlsaInterface();
};
