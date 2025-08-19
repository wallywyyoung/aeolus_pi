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
#include "aeolus/StopFactory.h"
#include "aeolus/Organ.h"

void DivisionFactory::initFromJson(const std::shared_ptr<VoicePool> voicePool, std::vector<std::unique_ptr<Division>> &divisions, std::function<RankWave *(const std::string &)> getStopByName) {
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
        auto division = DivisionFactory::initFromJson(divisionDef, voicePool, getStopByName);
        divisions.push_back(std::move(division));
    }
    for (auto& division : divisions) {
        for (auto &divisionName : division->_linkedDivisionNames) {
            auto it = std::ranges::find_if(divisions,[&](const std::unique_ptr<Division>& d) { return d->_name == divisionName; });
            if (it != divisions.end()) {
                division->_linkedDivisions.push_back(Division::Coupler{ it->get(), false });
                it->get()->_linkedFromDivisions.push_back(division.get());
            }
        }
    }
}

std::unique_ptr<Division> DivisionFactory::initFromJson(nlohmann::json &json, std::shared_ptr<VoicePool> voicePool, std::function<RankWave *(const std::string &)> getStopByName) {
    auto division = std::make_unique<Division>();

    division->_name = json["name"];
    division->_mnemonic = json["mnemonic"];

    if (json.contains("link")) {
        division->_linkedDivisionNames.reserve(json.count("link"));
        if (const auto link = json["link"]; link.is_array()) {
            for (const auto& item : link)
                division->_linkedDivisionNames.push_back(item);
        }
    }

    division->_voicePool = voicePool;

    division->_hasSwell = json.contains("swell") && json["swell"];
    division->_hasTremulant = json.contains("tremulant") && json["tremulant"];
    division->_tremulantLevel.setRange(0.0f, division->_hasTremulant ? static_cast<float>(json["tremulant_level"]) : 0.0f);

    StopFactory::initFromJson(json, division->_stops, getStopByName);

    return std::move(division);
}
