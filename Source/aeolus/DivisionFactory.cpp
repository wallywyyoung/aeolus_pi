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

#include "DivisionFactory.h"

#include "engine.h"

std::unique_ptr<Division> DivisionFactory::initFromJson(nlohmann::json& v, const Engine& engine, const std::string& name) {
    auto division = std::make_unique<Division>(engine, name);

    division->_name = v["name"];
    division->_mnemonic = v["mnemonic"];

    if (v.contains("link")) {
        if (const auto link = v["link"]; link.is_array()) {
            for (const auto& item : link)
                division->_linkedDivisionNames.push_back(item);
        }
    }

    division->_hasSwell = v.contains("swell") && v["swell"];
    division->_hasTremulant = v.contains("tremulant") && v["tremulant"];
    division->_tremulantMaxLevel = division->_hasTremulant ? static_cast<float>(v["tremulant_level"]) : 0.0f;

    if (const auto arr = v["stops"]; arr.is_array()) {
        auto stop = Stop();
        for (const auto& item : arr) {
            stop.initFromJson(item);
            if (!stop.getZones().empty())
               division->_stops.push_back(stop);
        }
    }

    return std::move(division);
}
