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
#include "EngineGlobal.h"
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
    std::ostringstream path;
    for (auto jsonIr: jsonIRs["irs"]){
        auto zeroDelay = jsonIr.contains("zeroDelay") ? static_cast<bool>(jsonIr["zeroDelay"]) : false;
        auto startOffset = zeroDelay? 0 : static_cast<int>(jsonIr["startOffset"]);
        audioFile.load("./Resources/irs/" + std::to_string(SAMPLE_RATE) + "/" + static_cast<std::string>(jsonIr["fileName"]));
        auto ir = IR(jsonIr["name"], audioFile, startOffset);
        //TODO: This must be wrong.
        auto gain = static_cast<float>(jsonIr["gain"]);
        ir.applyGain(gain);
        irs.irs.push_back(ir);
        irs.longestIRLength = std::max(irs.longestIRLength, audioFile.getNumSamplesPerChannel());
        path.clear();
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
            continue;
        }
        auto extension = entry.path().extension().string();
        auto synth = Addsynth();
        if (extension == ".ae0") {
            addsynthFromBinary(entry, synth);
        } else if (extension == ".json") {
            addsynthFromJson(entry, synth);
        } else {
            std::cout << "Skipping Addsynth file " << entry.path() << std::endl;
            continue;
        }
        synths.push_back(synth);
    }
    return synths;
}

void IOManager::HN_func_fromJson(HN_func& hnFunc, nlohmann::json& v) {
    if (v.size() < hnFunc._h.size()) {
        return;
    }
    for (int i = 0; i < hnFunc._h.size(); ++i) {
        N_func_fromJson(hnFunc._h[i], v[i]);
    }
}

void IOManager::HN_func_fromStream(HN_func& hnFunc, std::istream& stream, const int &nHarm){
    const auto m = std::min(hnFunc._h.size(), static_cast<size_t>(nHarm));
    for (int i = 0; i < m; ++i) {
        N_func_fromStream(hnFunc._h[i], stream);
    }
}


void IOManager::N_func_fromJson(N_func &nFunc, const nlohmann::json& v) {
    nFunc._b = v["mask"];
    if (auto varr = v["values"]; varr.is_array()) {
        if (varr.size() >= nFunc._v.size()) {
            for (int i = 0; i < nFunc._v.size(); ++i)
                nFunc._v[i] = varr[i];
        }
    }
}

void IOManager::N_func_fromStream(N_func &nFunc, std::istream& stream)
{
    stream.read(reinterpret_cast<char*>(&nFunc._b), sizeof(int));

    for (int i = 0; i < nFunc._v.size(); ++i) {
        stream.read(reinterpret_cast<char*>(&nFunc._v[i]), sizeof(float));
    }
}

void IOManager::addsynthFromJson(const std::filesystem::directory_entry& entry, Addsynth &addsynth) {
    std::ifstream stream(entry.path(), std::ios::in);
    auto v = nlohmann::json::parse(stream);

    // TODO: Fix this hack.
    const auto tempPath = entry.path().filename().stem().string();
    addsynth._fileName = tempPath.substr(0, tempPath.length() - 4);

    const int version = v["version"];

    addsynth._noteMin = v["note_min"];
    addsynth._noteMax = v["note_max"];

    if (addsynth._noteMax == deprecated::NOTE_MAX) {
        addsynth._noteMax = Addsynth::NOTE_MAX;
    }

    addsynth._fn = v["fn"];
    addsynth._fd = v["fd"];

    addsynth._stopName = v["name"];
    addsynth._copyright = v["copyright"];
    addsynth._mnemonic = v["mnemonic"];
    addsynth._comments = v["comments"];

    N_func_fromJson(addsynth._n_vol, v["n_vol"]);
    N_func_fromJson(addsynth._n_off, v["n_off"]);
    N_func_fromJson(addsynth._n_ran, v["n_ran"]);

    if (version >= Addsynth::defaultVersion) {
        N_func_fromJson(addsynth._n_ins, v["n_ins"]);
        N_func_fromJson(addsynth._n_att, v["n_att"]);
        N_func_fromJson(addsynth._n_atd, v["n_atd"]);
        N_func_fromJson(addsynth._n_dct, v["n_dct"]);
        N_func_fromJson(addsynth._n_dcd, v["n_dcd"]);
    }

    HN_func_fromJson(addsynth._h_lev, v["h_lev"]);
    HN_func_fromJson(addsynth._h_ran, v["h_ran"]);
    HN_func_fromJson(addsynth._h_att, v["h_att"]);
    HN_func_fromJson(addsynth._h_atp, v["h_atp"]);
}

void IOManager::addsynthFromBinary(const std::filesystem::directory_entry& entry, Addsynth &addsynth)
{
    std::ifstream stream(entry.path(), std::ios::in | std::ios::binary);
    addsynth._fileName = entry.path().filename().stem();
    char header[Addsynth::header_length]{};

    stream.read(header, Addsynth::header_length);

    if (strncmp(header, "AEOLUS", 6) != 0)
        throw std::runtime_error("Invalid header signature");

    const int version = header[7];
    int nHarm = header[26];

    if (nHarm == 0) {
        nHarm = deprecated::N_HARM;
    }

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

    N_func_fromStream(addsynth._n_vol, stream);
    N_func_fromStream(addsynth._n_off, stream);
    N_func_fromStream(addsynth._n_ran, stream);

    if (version >= Addsynth::defaultVersion) {
        N_func_fromStream(addsynth._n_ins, stream);
        N_func_fromStream(addsynth._n_att, stream);
        N_func_fromStream(addsynth._n_atd, stream);
        N_func_fromStream(addsynth._n_dct, stream);
        N_func_fromStream(addsynth._n_dcd, stream);
    }

    HN_func_fromStream(addsynth._h_lev, stream, nHarm);
    HN_func_fromStream(addsynth._h_ran, stream, nHarm);
    HN_func_fromStream(addsynth._h_att, stream, nHarm);
    HN_func_fromStream(addsynth._h_atp, stream, nHarm);
}
