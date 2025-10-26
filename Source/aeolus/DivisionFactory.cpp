// ----------------------------------------------------------------------------
//
//  Copyright (C) 2025 Wally Young <wallywyyoung@users.noreply.github.com>
//  Copyright (C) 2021 Arthur Benilov <arthur.benilov@gmail.com>
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

#include "aeolus/DivisionFactory.h"
#include <filesystem>
#include "aeolus/Organ.h"
#include "aeolus/StopFactory.h"

void DivisionFactory::initFromJson(std::vector<std::shared_ptr<Division>> &divisions, std::function<std::shared_ptr<RankWave>(const std::string &)> getStopByName, std::shared_ptr<VoicePool<>> voicePool) {
    const std::filesystem::path configFile = "./Resources/configs/default_organ.json";
    if (!exists(configFile)) {
        return;
    }
    std::ifstream stream(configFile);
    auto json = nlohmann::json::parse(stream);
    if (!json["divisions"].is_array()) {
        return;
    }
    divisions.reserve(json.count("divisions"));
    for (auto divisionDef : json["divisions"]) {
        auto division = initFromJson(divisionDef, getStopByName);
        division->voicePool = voicePool;
        divisions.push_back(std::move(division));
    }
    for (auto& division : divisions) {
        for (auto &divisionName : division->linkedDivisionNames) {
            auto it = std::ranges::find_if(divisions,[&](const std::shared_ptr<Division>& d) { return d->name == divisionName; });
            if (it != divisions.end()) {
                auto coupler = std::make_shared<Division::Coupler>(*it, false);
                division->linkedDivisions.push_back(coupler);
                it->get()->linkedFromDivisions.push_back(coupler);
            }
        }
    }
}

std::shared_ptr<Division> DivisionFactory::initFromJson(nlohmann::json &json, std::function<std::shared_ptr<RankWave>(const std::string &)> getStopByName) {
    auto division = std::make_shared<Division>();

    division->name = json["name"];
    division->mnemonic = json["mnemonic"];

    if (json.contains("link")) {
        division->linkedDivisionNames.reserve(json.count("link"));
        if (const auto link = json["link"]; link.is_array()) {
            for (const auto& item : link)
                division->linkedDivisionNames.push_back(item);
        }
    }

    division->hasSwell = json.contains("swell") && json["swell"];
    division->hasTremulant = json.contains("tremulant") && json["tremulant"];
    division->tremulantLevel.setRange(0.0f, division->hasTremulant ? static_cast<float>(json["tremulant_level"]) : 0.0f);

    StopFactory::initFromJson(json, division->stops, getStopByName);

    return std::move(division);
}
