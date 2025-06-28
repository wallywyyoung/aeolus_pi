// ----------------------------------------------------------------------------
//
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

#include "aeolus/globals.h"
#include "HN_func.h"

#include <cstdint>
#include <array>
#include <variant>
#include <map>
#include <any>
#include <memory>

#include <nlohmann/json.hpp>

AEOLUS_NAMESPACE_BEGIN

class Addsynth final
{
public:
    Addsynth();

    void reset();

    std::string getStopName() const { return _stopName; }
    void setStopName(const std::string& n) { _stopName = n; }
    std::string getCopyright() const { return _copyright; }
    std::string getMnemonic() const { return _mnemonic; }
    std::string getComments() const { return _comments; }

    int getNoteMin() const noexcept { return _noteMin; }
    int getNoteMax() const noexcept { return _noteMax; }

    void fromJson(const nlohmann::json& v);

    void fromStream(std::istream& stream);

    float getNoteVolume(int n) const noexcept { return _n_vol[n]; }
    float getNoteAttack(int n) const noexcept { return _n_att[n]; }
    float getNoteOffset(int n) const noexcept { return _n_off[n]; }
    float getNoteRandomisation(int n) const noexcept { return _n_ran[n]; }
    float getNoteInstability(int n) const noexcept { return _n_ins[n]; }
    float getNoteAttackDetune(int n) const noexcept { return _n_atd[n]; }
    float getNoteRelease(int n) const noexcept { return _n_dct[n]; }
    float getNoteReleaseDetune(int n) const noexcept { return _n_dcd[n]; }

    float getHarmonicLevel(int h, int n) const noexcept { return _h_lev[h][n]; }
    float getHarmonicAttack(int h, int n) const noexcept { return _h_att[h][n]; }
    float getHarmonicRandomisation(int h, int n) const noexcept { return _h_ran[h][n]; }
    float getHarmonicAttackProfile(int h, int n) const noexcept { return _h_atp[h][n]; }

    /// Frequency ration nominator.
    int getFn() const noexcept { return _fn; }

    ///  Frequency ratio denominator.
    int getFd() const noexcept { return _fd; }

private:

    constexpr static int defaultVersion = 2;

    constexpr static size_t header_length    = 32;
    constexpr static size_t stopName_length  = 32;
    constexpr static size_t copyright_length = 56;
    constexpr static size_t mnemonic_length  = 8;
    constexpr static size_t comments_length  = 56;
    constexpr static size_t reserved_length  = 8;

    std::string _stopName;
    std::string _copyright;
    std::string _mnemonic;
    std::string _comments;

    int _noteMin;   // _n0;
    int _noteMax;   // _n1;
    int _fn;
    int _fd;

    N_func  _n_vol;
    N_func  _n_off;
    N_func  _n_ran;
    N_func  _n_ins;
    N_func  _n_att; ///< Attack time.
    N_func  _n_atd; ///< Attack detune.
    N_func  _n_dct; ///< Releate time.
    N_func  _n_dcd; ///< Release detune.

    HN_func _h_lev; ///< Harmonic level
    HN_func _h_ran; ///< Harmonic level randomization.
    HN_func _h_att; ///< Harmonic attack time
    HN_func _h_atp; ///< Harmonic attack profile.
};

AEOLUS_NAMESPACE_END
