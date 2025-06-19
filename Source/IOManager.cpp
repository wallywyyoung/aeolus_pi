//
// Created by Wally Young on 6/6/25.
//

#include "IOManager.h"
#include "aeolus/dsp/convolver.h"
#include "aeolus/addsynth.h"
#include "aeolus/division.h"
#include "aeolus/EngineGlobal.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <AudioFile/AudioFile.h>
#include <algorithm>
#include <filesystem>

IRs IOManager::loadIRs() {
    std::ifstream stream("irs.json");
    auto jsonIRs = nlohmann::json::parse(stream);
    IRs irs;
    irs.longestIRLength = aeolus::dsp::Convolver::BlockSize;
    AudioFile<float> audioFile;
    IR ir;
    for (auto jsonIr: jsonIRs["irs"]){
        audioFile.load(jsonIr["fileName"]);
        ir.waveform.setBuffer(audioFile.samples);
        ir.channelSamples = audioFile.getNumSamplesPerChannel();
        auto startOffset = static_cast<int>(jsonIr["startOffset"]);
        auto gain = static_cast<float>(jsonIr["gain"]);
        for (auto channel : ir.waveform) {
            channel.erase(channel.begin(), channel.begin() + startOffset);
            std::transform(channel.begin(), channel.end(), channel.begin(), [&](float element) { return element * gain; });
        }
        irs.irs.push_back(ir);
        irs.longestIRLength = std::max(irs.longestIRLength, audioFile.getNumSamplesPerChannel());
    }
    return irs;
}


//void IOManager::loadExternalPipes()
//{
//    std::ifstream stream("organ_config.json");
//    auto jsonConfig = nlohmann::json::parse(stream);
//
//    const String configFileName{ configFile.getFileName() };
//    File configFolder{ configFile.getParentDirectory() };
//
//    if (!configFolder.exists())
//        return;
//
//    for (DirectoryEntry entry : RangedDirectoryIterator(configFolder, true)) {
//        auto file{ entry.getFile() };
//        const auto ext{ file.getFileExtension().toLowerCase() };
//
//        if ((ext == ".json" && file.getFileName() != configFileName) || (ext == ".ae0")) {
//            auto synth = std::make_unique<Addsynth>();
//            const auto res{ synth->readFromFile(file) };
//
//            if (res.wasOk()) {
//                String stopName{ file.getFileNameWithoutExtension() };
//                synth->setStopName(stopName);
//
//                addSynth(std::move(synth));
//            } else {
//                // TODO: Search and replace all DBGs
////                DBG("Failed to read: " << res.getErrorMessage());
//            }
//        }
//    }
//}

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

std::vector<aeolus::Addsynth> IOManager::loadPipes()
{
    constexpr const char* directory = "./Resources/stops/";
    constexpr const char* binaryExtension = ".ae0";
    constexpr const char* jsonExtension = ".json";
    std::vector<aeolus::Addsynth> synths;

    for (const auto& entry : std::filesystem::directory_iterator(directory)) {
        if (!std::filesystem::is_regular_file(entry)) {
            continue;
        }
        auto extension = entry.path().extension().string();
        auto synth = aeolus::Addsynth();
        if (extension.compare(binaryExtension)) {
            auto binary = readBinaryFile(entry.path());
            std::string binaryString(reinterpret_cast<const char*>(binary.data()), binary.size());
            std::istringstream stream(binaryString);
            synth.fromStream(stream);
            synths.push_back(synth);
        } else if (extension.compare(jsonExtension)) {
            std::ifstream stream(entry.path());
            auto json = nlohmann::json::parse(stream);
            synth.fromJson(json);
            synths.push_back(synth);
        }
    }

    return synths;
}
