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

#include "StopFactory.h"

#include "../EngineGlobal.h"

std::vector<RankWave *> StopFactory::getRankwavesFromPipeVar(const nlohmann::json &json, std::function<RankWave *(const std::string &)> stopByName) {
    std::vector<RankWave*> rankWaves;
    auto addRankwave = [&](const std::string& name) {
        if (const auto rankWave = stopByName(name)) {
            rankWaves.push_back(rankWave);
        } else {
            throw std::runtime_error("Stop pipe " + name + " cannot be found.");
        }
    };
    if (json.is_array()) {
        for (const auto& i : json) {
            addRankwave(i);
        }
    } else {
        const std::string pipeName = json;
        addRankwave(pipeName);
    }
    return rankWaves;
}

Stop::Type StopFactory::getTypeFromString(const std::string& n) {
    const static std::map<std::string, Stop::Type> nameToType {
                { "principal", Stop::Type::Principal },
                { "flute",     Stop::Type::Flute },
                { "reed",      Stop::Type::Reed },
                { "string",    Stop::Type::String }
    };

    auto type = Stop::Type::Unknown;

    // TODO: This is ugly, make cleaner.
    auto nameToFind = n;
    std::ranges::transform(nameToFind, nameToFind.begin(), ::tolower);

    if (const auto it = nameToType.find(nameToFind); it != nameToType.end()) {
        type = it->second;
    }

    return type;
}

void StopFactory::addZone(Stop &stop, const std::vector<RankWave *> &rw) {
    if (rw.empty()) {
        return;
    }
    Stop::Zone zone{};
    zone.keyRange = Range(rw[0]->getNoteMin(), rw[0]->getNoteMax() + 1);
    for (auto ptr : rw) {
        const Range range(ptr->getNoteMin(), ptr->getNoteMax() + 1);
        zone.keyRange = zone.keyRange.getUnionWith(range);
        zone.rankWaves.push_back(ptr);
    }
    stop.zones.push_back(zone);
}

void StopFactory::initFromJson(const nlohmann::json& json, Stop& stop, std::function<RankWave *(const std::string &)> getStopByName) {
    stop.name = json["name"];
    stop.type = getTypeFromString(json["type"]);

    if (json.contains("gain"))
        stop.gain = json["gain"];

    if (json.contains("chiff"))
        stop.chiffGain = json["chiff"];

    if (json.contains("pipe")) {
        const auto pipeObj = json["pipe"];
        if (const auto rankWaves{getRankwavesFromPipeVar(pipeObj, getStopByName)}; !rankWaves.empty())
            addZone(stop, rankWaves);

    } else if (!json["zones"].is_null()) {
        for (auto &zoneDef: json["zones"]) {
            Stop::Zone zone{};
            zone.rankWaves = getRankwavesFromPipeVar(zoneDef["pipe"], getStopByName);
            if (zoneDef.contains("range") && zoneDef["range"].is_array()) {
                zone.keyRange = Range(zoneDef["range"][0], zoneDef["range"][1]);
            }
            if (!zone.rankWaves.empty()) {
                stop.zones.push_back(zone);
            }
        }
    }
}

void StopFactory::initFromJson(const nlohmann::json &json, std::vector<Stop> &stops, std::function<RankWave *(const std::string &)> getStopByName) {
    if (const auto arr = json["stops"]; arr.is_array()) {
        stops.reserve(json.count("stops"));
        auto stop = Stop();
        for (const auto& item : arr) {
            initFromJson(item, stop, getStopByName);
            if (!stop.getZones().empty()) {
                stops.push_back(stop);
            }
        }
    }
}