// ----------------------------------------------------------------------------
//
//  Copyright (C) 2025 Wally Young <wallywyyoung@users.noreply.github.com>
//  Copyright (C) 2021 Arthur Benilov <arthur.benilov@gmail.com>
//  Copyright (C) 2003-2013 Fons Adriaensen <fons@linuxaudio.org>
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

#include "aeolus/Addsynth.h"
#include "aeolus/IR.h"

class IOManager {
public:
    static IRs loadIRs();
    static std::vector<Addsynth> loadPipes();
private:
    /// Values used by previous version of the synth.
    struct deprecated {
        constexpr static int N_HARM = 48;
        constexpr static int NOTE_MAX = 46;
    }; // namespace deprecated
    static std::vector<std::byte> readBinaryFile(const std::string &path);
    static void addsynthFromJson(const std::filesystem::directory_entry& entry, Addsynth &adsynth);
    static void addsynthFromBinary(const std::filesystem::directory_entry& entry, Addsynth &addsynth);
};
