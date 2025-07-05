// ----------------------------------------------------------------------------
//
//  Copyright (C) 2025 Wally Young <wallywyyoung@users.noreply.github.com>
//  Copyright (C) 2021 Arthur Benilov <arthur.benilov@gmail.com>
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

#include "AudioBuffer.h"

#include <string>
#include <vector>

class IR : public AudioBuffer {
    std::string name;

public:
    IR() : name{}, AudioBuffer() { }
    IR(std::string name, int channels, int bufferSize) : name(name), AudioBuffer(channels, bufferSize)  { }
    IR(const IR& other) = default;
    bool zeroDelay;

    void clear() {
        name.clear();
        AudioBuffer::clear();
    };
};

struct IRs {
    std::vector<IR> irs;
    int longestIRLength;
};