//
// Created by Wally Young on 6/6/25.
//

#include "IOManager.h"
#include "aeolus/Addsynth.h"
#include "aeolus/division.h"
#include "aeolus/EngineGlobal.h"
#include "aeolus/dsp/convolver.h"
#include <fstream>
#include <algorithm>
#include <filesystem>
#include <AudioFile/AudioFile.h>
#include <nlohmann/json.hpp>

IRs IOManager::loadIRs() {
    std::ifstream stream("irs.json");
    auto jsonIRs = nlohmann::json::parse(stream);
    IRs irs;
    irs.longestIRLength = dsp::Convolver::BlockSize;
    AudioFile<float> audioFile;
    IR ir;
    for (auto jsonIr: jsonIRs["irs"]){
        audioFile.load(jsonIr["fileName"]);
        ir.setBufferSize(audioFile.getNumSamplesPerChannel());
        ir.setBuffer(audioFile.samples);
        ir.zeroDelay = jsonIr["zeroDelay"];
        //TODO: This must be wrong.
        auto startOffset = ir.zeroDelay? 0 : static_cast<int>(jsonIr["startOffset"]);
        auto gain = static_cast<float>(jsonIr["gain"]);
        ir.applyGain(gain);
        irs.irs.push_back(ir);
        irs.longestIRLength = std::max(irs.longestIRLength, audioFile.getNumSamplesPerChannel());
    }
    return irs;
}

std::vector<std::byte> IOManager::readBinaryFile(std::string path) {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open file: " + path);
    }
    std::streamsize size = file.tellg();
    std::vector<std::byte> binary(size);
    if (!file.read(reinterpret_cast<char*>(binary.data()), size)) {
        throw std::runtime_error("Failed to read file: " + path);
    }
    return binary;
}

std::vector<Addsynth> IOManager::loadPipes() {
    std::vector<Addsynth> synths;
    for (const auto& entry : std::filesystem::directory_iterator(DIRECTORY)) {
        if (!std::filesystem::is_regular_file(entry)) {
            continue;
        }
        auto extension = entry.path().extension().string();
        auto synth = Addsynth();
        if (extension == BINARY_EXTENSION) {
            auto binary = IOManager::readBinaryFile(entry.path());
            std::string binaryString(reinterpret_cast<const char*>(binary.data()), binary.size());
            std::istringstream stream(binaryString);
            synth.fromStream(stream);
            synths.push_back(synth);
        } else if (extension == JSON_EXTENSION) {
            std::ifstream stream(entry.path());
            auto json = nlohmann::json::parse(stream);
            synth.fromJson(json);
            synths.push_back(synth);
        }
    }
    return synths;
}
