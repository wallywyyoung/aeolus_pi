//
// Created by Wally Young on 6/30/25.
//

#include "DivisionFactory.h"

std::unique_ptr<Division> DivisionFactory::initFromJson(nlohmann::json& v, const Engine& engine, const std::string& name) {
    auto division = std::make_unique<Division>(engine, name);

    division->_name = v["name"];
    division->_mnemonic = v["mnemonic"];

    if (v.contains("link")) {
        const auto link = v["link"];
        if (link.is_array()) {
            for (const auto& item : link)
                division->_linkedDivisionNames.push_back(item);
        }
    }

    division->_hasSwell = v.contains("swell") && v["swell"];
    division->_hasTremulant = v.contains("tremulant") && v["tremulant"];
    division->_tremulantMaxLevel = division->_hasTremulant ? static_cast<float>(v["tremulant_level"]) : 0.0f;

    const auto arr = v["stops"];
    if (arr.is_array()) {
        auto stop = Stop();
        for (const auto& item : arr) {
            stop.initFromJson(item);
            if (!stop.getZones().empty())
               division->_stops.push_back(stop);
        }
    }

    return std::move(division);
}
