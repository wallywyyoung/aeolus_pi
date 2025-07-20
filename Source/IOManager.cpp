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

#include "IOManager.h"
#include "aeolus/Addsynth.h"
#include "aeolus/Division.h"
#include "aeolus/EngineGlobal.h"
#include "aeolus/dsp/convolver.h"
#include <fstream>
#include <algorithm>
#include <filesystem>
#include <AudioFile/AudioFile.h>
#include <nlohmann/json.hpp>

IRs IOManager::loadIRs() {
    std::ifstream stream("./Resources/irs/irs.json");
    auto jsonIRs = nlohmann::json::parse(stream);
    IRs irs;
    irs.longestIRLength = dsp::Convolver::BlockSize;
    AudioFile<float> audioFile;
    IR ir;
    for (auto jsonIr: jsonIRs["irs"]){
        audioFile.load("./Resources/irs/" + static_cast<std::string>(jsonIr["fileName"]));
        ir.setBufferSize(audioFile.getNumSamplesPerChannel());
        ir.setBuffer(audioFile.samples);
        ir.zeroDelay = jsonIr.contains("zeroDelay") ? static_cast<bool>(jsonIr["zeroDelay"]) : false;
        //TODO: This must be wrong.
        auto startOffset = ir.zeroDelay? 0 : static_cast<int>(jsonIr["startOffset"]);
        auto gain = static_cast<float>(jsonIr["gain"]);
        ir.applyGain(gain);
        irs.irs.push_back(ir);
        irs.longestIRLength = std::max(irs.longestIRLength, audioFile.getNumSamplesPerChannel());
    }
    return irs;
}

std::vector<std::byte> IOManager::readBinaryFile(const std::string &path) {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open file: " + path);
    }
    const std::streamsize size = file.tellg();
    std::vector<std::byte> binary(size);
    if (!file.read(reinterpret_cast<char*>(binary.data()), size)) {
        throw std::runtime_error("Failed to read file: " + path);
    }
    return binary;
}

std::vector<Addsynth> IOManager::loadPipes() {
    std::vector<Addsynth> synths;
    for (const auto& entry : std::filesystem::directory_iterator("./Resources/stops/")) {
        if (!std::filesystem::is_regular_file(entry)) {
            std::cout << "Skipping rankwave file " << entry.path() << std::endl;
            continue;
        }
        auto extension = entry.path().extension().string();

        std::cout << "Loading rankwave file " << entry.path() << std::endl;
        auto synth = Addsynth();
        if (extension == ".ae0") {
            addsynthFromBinary(entry, synth);
        } else if (extension == ".json") {
            addsynthFromJson(entry, synth);
        }
        synths.push_back(synth);
    }
    return synths;
}


void IOManager::addsynthFromJson(const std::filesystem::directory_entry& entry, Addsynth &addsynth) {
    std::ifstream stream(entry.path(), std::ios::in);
    auto v = nlohmann::json::parse(stream);

    //TODO: Fix this hack.
    auto tempPath = entry.path().filename().stem().string();
    addsynth._fileName = tempPath.substr(0, tempPath.length() - 4);

    const int version = v["version"];

    addsynth._noteMin = v["note_min"];
    addsynth._noteMax = v["note_max"];

    if (addsynth._noteMax == deprecated::NOTE_MAX)
        addsynth._noteMax = Addsynth::NOTE_MAX;

    addsynth._fn = v["fn"];
    addsynth._fd = v["fd"];

    addsynth._stopName = v["name"];
    addsynth._copyright = v["copyright"];
    addsynth._mnemonic = v["mnemonic"];
    addsynth._comments = v["comments"];

    addsynth._n_vol.fromJson(v["n_vol"]);
    addsynth._n_off.fromJson(v["n_off"]);
    addsynth._n_ran.fromJson(v["n_ran"]);

    if (version >= Addsynth::defaultVersion) {
        addsynth._n_ins.fromJson(v["n_ins"]);
        addsynth._n_att.fromJson(v["n_att"]);
        addsynth._n_atd.fromJson(v["n_atd"]);
        addsynth._n_dct.fromJson(v["n_dct"]);
        addsynth._n_dcd.fromJson(v["n_dcd"]);
    }

    addsynth._h_lev.reset(-100.0f);
    addsynth._h_ran.reset(0.0f);
    addsynth._h_att.reset(0.050f);
    addsynth._h_atp.reset(0.0f);

    addsynth._h_lev.fromJson(v["h_lev"]);
    addsynth._h_ran.fromJson(v["h_ran"]);
    addsynth._h_att.fromJson(v["h_att"]);
    addsynth._h_atp.fromJson(v["h_atp"]);
}

void IOManager::addsynthFromBinary(const std::filesystem::directory_entry& entry, Addsynth &addsynth)
{
    std::ifstream stream(entry.path(), std::ios::in | std::ios::binary);
    addsynth._fileName = entry.path().filename().stem();
    char header[Addsynth::header_length] = {0};

    stream.read(header, Addsynth::header_length);

    if (strncmp(header, "AEOLUS", 6) != 0)
        throw std::runtime_error("Invalid header signature");

    const int version = header[7];
    int nHarm = header[26];

    if (nHarm == 0)
        nHarm = deprecated::N_HARM;

    addsynth._noteMin = header[28];
    addsynth._noteMax = header[29];

    if (addsynth._noteMax == deprecated::NOTE_MAX)
        addsynth._noteMax = Addsynth::NOTE_MAX;

    addsynth._fn = header[30];
    addsynth._fd = header[31];

    char ch;
    while (stream.get(ch) && ch != '\0') { // Read character by character until null or EOF
        addsynth._stopName += ch;
    }
    while (stream.get(ch) && ch == '\0') { }
    addsynth._copyright += ch;
    while (stream.get(ch) && ch != '\0') { // Read character by character until null or EOF
        addsynth._copyright += ch;
    }
    while (stream.get(ch) && ch == '\0') { }
    addsynth._mnemonic += ch;
    while (stream.get(ch) && ch != '\0') { // Read character by character until null or EOF
        addsynth._mnemonic += ch;
    }
    while (stream.get(ch) && ch == '\0') { }
    addsynth._comments += ch;
    while (stream.get(ch) && ch != '\0') { // Read character by character until null or EOF
        addsynth._comments += ch;
    }

    stream.seekg(Addsynth::data_offset, std::ios::beg);

    addsynth._n_vol.read(stream);
    addsynth._n_off.read(stream);
    addsynth._n_ran.read(stream);

    if (version >= Addsynth::defaultVersion) {
        addsynth._n_ins.read(stream);
        addsynth._n_att.read(stream);
        addsynth._n_atd.read(stream);
        addsynth._n_dct.read(stream);
        addsynth._n_dcd.read(stream);
    }

    addsynth._h_lev.reset(-100.0f);
    addsynth._h_ran.reset(0.0f);
    addsynth._h_att.reset(0.050f);
    addsynth._h_atp.reset(0.0f);

    addsynth._h_lev.read(stream, nHarm);
    addsynth._h_ran.read(stream, nHarm);
    addsynth._h_att.read(stream, nHarm);
    addsynth._h_atp.read(stream, nHarm);
}
