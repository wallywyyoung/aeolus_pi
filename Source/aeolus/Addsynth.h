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
// ---------------------------------------------------------------------------

#pragma once

#include "aeolus/HN_func.h"

#include <nlohmann/json.hpp>

class Addsynth final
{
public:
    explicit Addsynth() {
        reset();
    }

    void reset();

    void setStopName(const std::string& n) { _stopName = n; }

    [[nodiscard]] const std::string& getStopName() const { return _stopName; }
    [[nodiscard]] const std::string& getFileName() const { return _fileName; }
    [[nodiscard]] const std::string& getCopyright() const { return _copyright; }
    [[nodiscard]] const std::string& getMnemonic() const { return _mnemonic; }
    [[nodiscard]] const std::string& getComments() const { return _comments; }
    [[nodiscard]] int getNoteMin() const noexcept { return _noteMin; }
    [[nodiscard]] int getNoteMax() const noexcept { return _noteMax; }

    void fromJson(const nlohmann::json& v);

    void fromStream(std::istream& stream);

    [[nodiscard]] float getNoteVolume(const int n) const noexcept { return _n_vol[n]; }
    [[nodiscard]] float getNoteAttack(const int n) const noexcept { return _n_att[n]; }
    [[nodiscard]] float getNoteOffset(const int n) const noexcept { return _n_off[n]; }
    [[nodiscard]] float getNoteRandomisation(const int n) const noexcept { return _n_ran[n]; }
    [[nodiscard]] float getNoteInstability(const int n) const noexcept { return _n_ins[n]; }
    [[nodiscard]] float getNoteAttackDetune(const int n) const noexcept { return _n_atd[n]; }
    [[nodiscard]] float getNoteRelease(const int n) const noexcept { return _n_dct[n]; }
    [[nodiscard]] float getNoteReleaseDetune(const int n) const noexcept { return _n_dcd[n]; }
    [[nodiscard]] float getHarmonicLevel(const int h, const int n) const noexcept { return _h_lev[h][n]; }
    [[nodiscard]] float getHarmonicAttack(const int h, const int n) const noexcept { return _h_att[h][n]; }
    [[nodiscard]] float getHarmonicRandomisation(const int h, const int n) const noexcept { return _h_ran[h][n]; }
    [[nodiscard]] float getHarmonicAttackProfile(const int h, const int n) const noexcept { return _h_atp[h][n]; }

    /// Frequency ration nominator.
    [[nodiscard]] int getFn() const noexcept { return _fn; }

    ///  Frequency ratio denominator.
    [[nodiscard]] int getFd() const noexcept { return _fd; }

private:
    /// Lowest possible note.
    constexpr static int NOTE_MIN = 36;
    /// Highest possible note.
    constexpr static int NOTE_MAX = 96;

    constexpr static int defaultVersion = 2;

    constexpr static size_t header_length    = 32;
    constexpr static size_t stopName_length  = 32;
    constexpr static size_t copyright_length = 56;
    constexpr static size_t mnemonic_length  = 8;
    constexpr static size_t comments_length  = 56;
    constexpr static size_t reserved_length  = 8;
    constexpr static size_t data_offset = header_length + stopName_length + copyright_length + mnemonic_length + comments_length + reserved_length;

    std::string _fileName{};
    std::string _stopName{};
    std::string _copyright{};
    std::string _mnemonic{};
    std::string _comments{};

    int _noteMin{};   // _n0;
    int _noteMax{};   // _n1;
    int _fn{};
    int _fd{};

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

    friend class IOManager;
};


