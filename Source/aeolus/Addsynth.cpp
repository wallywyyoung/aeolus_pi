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

#include "aeolus/Addsynth.h"
#include <fstream>
#include <cassert>
#include <map>
#include <cstring>

template <typename T>
static inline bool isPositiveAndBelow(T valueToTest, T upperLimit) {
    return valueToTest >= 0 && valueToTest < upperLimit;
}



Addsynth::Addsynth()
{
    reset();
}

void Addsynth::reset()
{
    _noteMin = NOTE_MIN;
    _noteMax = NOTE_MAX;
    _fn = 1;
    _fd = 1;

    _n_vol.reset(-20.0f);
    _n_ins.reset(0.0f);
    _n_off.reset(0.0f);
    _n_att.reset(0.01f);
    _n_atd.reset(0.0f);
    _n_dct.reset(0.01f);
    _n_dcd.reset(0.0f);
    _n_ran.reset(0.0f);
    _h_lev.reset(-100.0f);
    _h_ran.reset(0.0f);
    _h_att.reset(0.050f);
    _h_atp.reset(0.0f);
}

void Addsynth::fromJson(const nlohmann::json& v)
{
    int version = v["version"];

    _noteMin = v["note_min"];
    _noteMax = v["note_max"];

    if (_noteMax == deprecated::NOTE_MAX)
        _noteMax = NOTE_MAX;

    _fn = v["fn"];
    _fd = v["fd"];

    _stopName = v["name"];
    _copyright = v["copyright"];
    _mnemonic = v["mnemonic"];
    _comments = v["comments"];

    _n_vol.fromJson(v["n_vol"]);
    _n_off.fromJson(v["n_off"]);
    _n_ran.fromJson(v["n_ran"]);

    if (version >= defaultVersion) {
        _n_ins.fromJson(v["n_ins"]);
        _n_att.fromJson(v["n_att"]);
        _n_atd.fromJson(v["n_atd"]);
        _n_dct.fromJson(v["n_dct"]);
        _n_dcd.fromJson(v["n_dcd"]);
    }

    _h_lev.reset(-100.0f);
    _h_ran.reset(0.0f);
    _h_att.reset(0.050f);
    _h_atp.reset(0.0f);

    _h_lev.fromJson(v["h_lev"]);
    _h_ran.fromJson(v["h_ran"]);
    _h_att.fromJson(v["h_att"]);
    _h_atp.fromJson(v["h_atp"]);
}

void Addsynth::fromStream(std::istream& stream)
{
    char header[header_length] = {0};

    stream.read(header, header_length);

    if (strncmp(header, "AEOLUS", 6) != 0)
        throw std::runtime_error("Invalid header signature");

    int version = header[7];
    int nHarm = header[26];

    if (nHarm == 0)
        nHarm = deprecated::N_HARM;

    _noteMin = header[28];
    _noteMax = header[29];

    if (_noteMax == deprecated::NOTE_MAX)
        _noteMax = NOTE_MAX;

    _fn = header[30];
    _fd = header[31];

    std::string reserved;

    _stopName.reserve(stopName_length);
    _copyright.reserve(copyright_length);
    _mnemonic.reserve(mnemonic_length);
    _comments.reserve(comments_length);
    reserved.reserve(comments_length);

    stream.read(_stopName.data(), stopName_length);
    stream.read(_copyright.data(), copyright_length);
    stream.read(_mnemonic.data(), mnemonic_length);
    stream.read(_comments.data(), comments_length);
    stream.read(reserved.data(), reserved_length);

    _stopName.shrink_to_fit();
    _copyright.shrink_to_fit();
    _mnemonic.shrink_to_fit();
    _comments.shrink_to_fit();
    reserved.shrink_to_fit();

    _n_vol.read(stream);
    _n_off.read(stream);
    _n_ran.read(stream);

    if (version >= defaultVersion) {
        _n_ins.read(stream);
        _n_att.read(stream);
        _n_atd.read(stream);
        _n_dct.read(stream);
        _n_dcd.read(stream);
    }

    _h_lev.reset(-100.0f);
    _h_ran.reset(0.0f);
    _h_att.reset(0.050f);
    _h_atp.reset(0.0f);

    _h_lev.read(stream, nHarm);
    _h_ran.read(stream, nHarm);
    _h_att.read(stream, nHarm);
    _h_atp.read(stream, nHarm);
}
