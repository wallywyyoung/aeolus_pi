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

#pragma once

#ifdef MACOS

#include <RtAudio.h>
#include <functional>
#include "MemoryConstants.h"

class RtAudioInterface final {
public:
    ~RtAudioInterface() = default;
    explicit RtAudioInterface(const std::function<void(float (&out)[PROCESS_SAMPLES_SIZE])> &processAudio);

private:
    std::function<void(float (&out)[PROCESS_SAMPLES_SIZE])> processAudio;
    RtAudio* device;
    static int audioHandler(void *outputBuffer, void *inputBuffer, unsigned int nFrames, double streamTime,
                            RtAudioStreamStatus status, void *userData);
    void endPlayback();
};

#endif
