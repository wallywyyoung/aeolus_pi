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

#ifdef MACOS

#include "RtAudioInterface.h"

#include <RtAudio.h>
#include <stdexcept>
#include "MemoryConstants.h"
#include "aeolus/utilities/SimdUtilities.h"

RtAudioInterface::RtAudioInterface(std::function<void(float (&out)[PROCESS_SAMPLES_SIZE])> processAudio) : processAudio(processAudio) {
    const auto sp = new RtAudio::StreamParameters();
    try {
        device = new RtAudio(RtAudio::MACOSX_CORE);
        sp->deviceId = device->getDefaultOutputDevice();
        sp->nChannels = OUTPUT_CHANNELS;
        sp->firstChannel = 0;
        auto frames = static_cast<unsigned int>(PROCESS_FRAMES_SIZE);
        device->openStream(sp, nullptr, RTAUDIO_FLOAT32, SAMPLE_RATE, &frames, audioHandler, this);
    } catch (const std::exception &e) {
        throw std::runtime_error(std::string("RtAudioInterface::initAudio(): ") + e.what());
    }
    delete sp;
    device->startStream();
}

int RtAudioInterface::audioHandler(void *outputBuffer, void *inputBuffer, unsigned int nFrames, double streamTime, RtAudioStreamStatus status, void *userData) {
    SimdUtilities::enableFlushToZero();
    if (nFrames < PROCESS_FRAMES_SIZE) {
        return 1;
    }
    const auto a = static_cast<RtAudioInterface*>(userData);
    auto* output = static_cast<float(*)[PROCESS_SAMPLES_SIZE]>(outputBuffer);
    a->processAudio(*output);

    SimdUtilities::disableFlushToZero();
    return 0;
}

void RtAudioInterface::endPlayback() {
    device->stopStream();
    device->closeStream();
    delete device;
}

#endif
