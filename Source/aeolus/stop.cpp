// ----------------------------------------------------------------------------
//
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

#include "aeolus/stop.h"
#include "aeolus/EngineGlobal.h"



Stop::Stop() = default;

std::vector<std::shared_ptr<Rankwave>> getRankwavesFromPipeVar(const nlohmann::json& v) {
    std::vector<std::shared_ptr<Rankwave>> rankwaves;
    auto addRankwave = [&](const std::string& name) {
        if (const auto rankwave = EngineGlobal::getInstance().getStopByName(name)) {
            rankwaves.push_back(rankwave);
        } else {
            throw std::runtime_error("Stop pipe " + name + " cannot be found.");
        }
    };


    if (v.is_array()) {
        for (const auto& i : v) {
            addRankwave(i);
        }
    } else {
        const std::string pipeName = v;
        addRankwave(pipeName);
    }

    return rankwaves;
}

void Stop::initFromJson(const nlohmann::json& v) {
        _name = v["name"];
        _type = getTypeFromString(v["type"]);

        if (!v["gain"].is_null())
            _gain = v["gain"];

        if (!v["chiff"].is_null())
            _chiffGain = v["chiff"];

        if (!v["pipe"].is_null()) {
            const auto pipeObj = v["pipe"];

            if (const auto rankwaves{getRankwavesFromPipeVar(pipeObj)}; !rankwaves.empty())
                addZone(rankwaves);

        } else if (!v["zones"].is_null()) {
            for (auto &zoneDef: v["zones"]) {
                    Zone zone{};
                    zone.rankwaves = getRankwavesFromPipeVar(zoneDef["pipe"]);

                    if (const auto range = zoneDef["range"]; range.is_array()) {
                        if (range.size() >= 2)
                            zone.keyRange = Range((int) range.front(), static_cast<int>(range.back()) + 1);
                    }

                    if (!zone.rankwaves.empty())
                        _zones.push_back(zone);
                }

        }
    }

float Stop::getGain() const noexcept { return _gain; }

void Stop::addZone(const std::shared_ptr<Rankwave> &ptr)
{
    assert(ptr != nullptr);

    Zone zone{};
    zone.keyRange = Range(ptr->getNoteMin(), ptr->getNoteMax() + 1);
    zone.rankwaves.push_back(ptr);

    _zones.push_back(zone);
}

void Stop::addZone(const std::vector<std::shared_ptr<Rankwave>> &rw)
{
    if (rw.empty()) {
        return;
    }

    Zone zone{};
    zone.keyRange = Range(rw[0]->getNoteMin(), rw[0]->getNoteMax() + 1);

    for (auto ptr : rw) {
        Range range(ptr->getNoteMin(), ptr->getNoteMax() + 1);
        zone.keyRange = zone.keyRange.getUnionWith(range);
        zone.rankwaves.push_back(ptr);
    }

    _zones.push_back(zone);
}

Range Stop::getKeyRange() const
{
    if (_zones.empty()) {
        return {};
    }

    auto range(_zones[0].keyRange);

    for (const auto& zone : _zones)
        range = range.getUnionWith(zone.keyRange);

    return range;
}

Stop::Type Stop::getTypeFromString(const std::string& n)
{
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
    const auto it = nameToType.find(nameToFind);

    if (it != nameToType.end()) {
        type = it->second;
    }

    return type;
}


