//
// Created by Wally Young on 6/6/25.
//

#include "IOManager.h"
#include "aeolus/dsp/convolver.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <AudioFile/AudioFile.h>
#include <algorithm>

IRs IOManager::loadIRs() {
    std::ifstream stream("irs.json");
    auto jsonIRs = nlohmann::json::parse(stream);
    IRs irs;
    irs.longestIRLength = aeolus::dsp::Convolver::BlockSize;
    AudioFile<float> audioFile;
    IR ir;
    for (auto jsonIr: jsonIRs["irs"]){
        audioFile.load(jsonIr["fileName"]);
        ir.waveform = audioFile.samples;
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

void IOManager::loadExternalPipes()
{
    std::ifstream stream("organ_config.json");
    auto jsonConfig = nlohmann::json::parse(stream);

    const String configFileName{ configFile.getFileName() };
    File configFolder{ configFile.getParentDirectory() };

    if (!configFolder.exists())
        return;

    for (DirectoryEntry entry : RangedDirectoryIterator(configFolder, true)) {
        auto file{ entry.getFile() };
        const auto ext{ file.getFileExtension().toLowerCase() };

        if ((ext == ".json" && file.getFileName() != configFileName) || (ext == ".ae0")) {
            auto synth = std::make_unique<Addsynth>();
            const auto res{ synth->readFromFile(file) };

            if (res.wasOk()) {
                String stopName{ file.getFileNameWithoutExtension() };
                synth->setStopName(stopName);

                addSynth(std::move(synth));
            } else {
                // TODO: Search and replace all DBGs
//                DBG("Failed to read: " << res.getErrorMessage());
            }
        }
    }
}

void IOManager::loadEmbeddedPipes()
{
    for (int i = 0; i < BinaryData::namedResourceListSize; ++i) {
        String filename(BinaryData::originalFilenames[i]);

        auto synth = std::make_unique<Addsynth>();
        String stopName;
        bool ok = false;

        if (filename.endsWith(".ae0")) {
            String name(BinaryData::namedResourceList[i]);
            auto res = synth->readFromResource(name);

            if (res.wasOk()) {
                ok = true;
                stopName = name.dropLastCharacters(4);
            }
        } else if (filename.endsWith("_ae0.json")) {
            String name(BinaryData::namedResourceList[i]);

            int size = 0;
            const char* data = BinaryData::getNamedResource(name.toRawUTF8(), size);

            if (data != nullptr) {
                MemoryInputStream stream(data, size, false);
                auto stop = JSON::parse(stream);
                synth->fromVar(stop);

                ok = true;
                stopName = name.dropLastCharacters(9);
            }
        }

        if (ok) {
            synth->setStopName(stopName);
            addSynth(std::move(synth));
        }
    }
}

void IOManager::populateDivisions()
{
    const auto configFile = getCustomOrganConfigFile();

    if (configFile.exists()) {
        FileInputStream stream(configFile);
        loadDivisionsFromConfig(stream);
    } else {
        MemoryInputStream stream(BinaryData::default_organ_json, BinaryData::default_organ_jsonSize, false);
        loadDivisionsFromConfig(stream);
    }

    // Remove all the links if any.
    for (auto division : _divisions) {
        division.clearLinkedDivisions();
    }

    // Update division links after they've been loaded.
    for (auto division : _divisions) {
        division.populateLinkedDivisions();
    }

    // @todo Do we want the divisions to be reordered by the couplings?
}

void IOManager::loadDivisionsFromConfig(std::ifstream& stream)
{
    // Load organ config JSON
    auto config = JSON::parse(stream);

    if (auto* divisions = config.getProperty("divisions", {}).getArray()) {
        for (int i = 0; i < divisions->size(); ++i) {
            if (auto* divisionObj = divisions->getUnchecked(i).getDynamicObject()) {
                auto division = std::make_unique<Division>(*this);

                division->initFromVar(divisions->getUnchecked(i));

                _divisions.add(division.release());
            }
        }
    }

    if (auto* sequencer = config.getProperty("sequencer", {}).getDynamicObject()) {
        if (var v = sequencer->getProperty("backward_key"); !v.isVoid()) {
            populateKeySwitchesVector(_sequencerStepBackwardKeySwitches, v);
        }

        if (var v = sequencer->getProperty("forward_key"); !v.isVoid()) {
            populateKeySwitchesVector(_sequencerStepForwardKeySwitches, v);
        }
    }
}