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

#include <string>

class AddSynth final {
public:
    explicit AddSynth() = default;
    void setStopName(const std::string& n) { stopName = n; }
    [[nodiscard]] const std::string& getStopName() const noexcept { return stopName; }
    [[nodiscard]] const std::string& getFileName() const noexcept { return fileName; }
    [[nodiscard]] const std::string& getCopyright() const noexcept { return copyright; }
    [[nodiscard]] const std::string& getMnemonic() const noexcept { return mnemonic; }
    [[nodiscard]] const std::string& getComments() const noexcept { return comments; }
    [[nodiscard]] int getNoteMinimum() const noexcept { return noteMinimum; }
    [[nodiscard]] int getNoteMaximum() const noexcept { return noteMaximum; }
    [[nodiscard]] float getNoteVolume(const int& n) const noexcept { return noteVolume[n]; }
    [[nodiscard]] float getNoteFrequencyOffset(const int& n) const noexcept { return noteFrequencyOffset[n]; }
    [[nodiscard]] float getNoteAttackTime(const int& n) const noexcept { return noteAttackTime[n]; }
    [[nodiscard]] float getNoteRandomisation(const int& n) const noexcept { return noteRandomisation[n]; }
    [[nodiscard]] float getNoteInstability(const int& n) const noexcept { return noteInstability[n]; }
    [[nodiscard]] float getNoteAttackDetune(const int& n) const noexcept { return noteAttackDetune[n]; }
    [[nodiscard]] float getNoteDecayTime(const int& n) const noexcept { return noteDecayTime[n]; }
    [[nodiscard]] float getNoteDecayDetune(const int& n) const noexcept { return noteDecayDetune[n]; }
    [[nodiscard]] float getHarmonicLevel(const int& h, const int& n) const noexcept { return harmonicLevel[h][n]; }
    [[nodiscard]] float getHarmonicAttack(const int& h, const int& n) const noexcept { return harmonicAttack[h][n]; }
    [[nodiscard]] float getHarmonicRandomisation(const int& h, const int& n) const noexcept { return harmonicRandomisation[h][n]; }
    [[nodiscard]] float getHarmonicAttackProfile(const int& h, const int& n) const noexcept { return harmonicAttackProfile[h][n]; }
    [[nodiscard]] int getFrequencyNumerator() const noexcept { return frequencyNumerator; } // Frequency ratio numerator.
    [[nodiscard]] int getFrequencyDenominator() const noexcept { return frequencyDenominator; } //  Frequency ratio denominator.

private:
    constexpr static auto NOTE_MINIMUM = 36; // Lowest possible note.
    constexpr static auto NOTE_MAXIMUM = 96; // Highest possible note.
    constexpr static auto DEFAULT_VERSION = 2;
    constexpr static size_t HEADER_LENGTH = 32;
    constexpr static size_t STOP_NAME_LENGTH = 32;
    constexpr static size_t COPYRIGHT_LENGTH = 56;
    constexpr static size_t MNEMONIC_LENGTH = 8;
    constexpr static size_t COMMENTS_LENGTH = 56;
    constexpr static size_t RESERVED_LENGTH = 8;
    constexpr static size_t DATA_OFFSET = HEADER_LENGTH + STOP_NAME_LENGTH + COPYRIGHT_LENGTH + MNEMONIC_LENGTH + COMMENTS_LENGTH + RESERVED_LENGTH;

    std::string fileName{};
    std::string stopName{};
    std::string copyright{};
    std::string mnemonic{};
    std::string comments{};

    int noteMinimum{NOTE_MINIMUM};   // _n0;
    int noteMaximum{NOTE_MAXIMUM};   // _n1;
    int frequencyNumerator{1};
    int frequencyDenominator{1};

    N_func  noteVolume{-20.0f};
    N_func  noteFrequencyOffset{0.0f};
    N_func  noteRandomisation{0.0f};
    N_func  noteInstability{0.0f};       ///< Note instability
    N_func  noteAttackTime{0.01f};       ///< Attack time.
    N_func  noteAttackDetune{0.0f};      ///< Attack detune.
    N_func  noteDecayTime{0.01f};        ///< Release time.
    N_func  noteDecayDetune{0.0f};       ///< Release detune.
    HN_func harmonicLevel{-100.0f};      ///< Harmonic level
    HN_func harmonicRandomisation{0.0f}; ///< Harmonic level randomization.
    HN_func harmonicAttack{0.05f};       ///< Harmonic attack time
    HN_func harmonicAttackProfile{0.0f}; ///< Harmonic attack profile.

    friend class IOManager;
};


