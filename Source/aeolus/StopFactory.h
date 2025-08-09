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

#pragma once

#include <vector>
#include <nlohmann/json.hpp>
#include <nlohmann/json_fwd.hpp>

#include "DivisionFactory.h"
#include "aeolus/Stop.h"

class StopFactory {
    static std::vector<Rankwave *> getRankwavesFromPipeVar(const nlohmann::json &json, std::function<Rankwave *(const std::string &)> stopByName);
    static Stop::Type getTypeFromString(const std::string& n);
    static void addZone(Stop &stop, const std::vector<Rankwave *> &rw);
    static void initFromJson(const nlohmann::json& json, Stop& stop, std::function<Rankwave *(const std::string &)> getStopByName);

public:
    static void initFromJson(const nlohmann::json& json, std::vector<Stop>& stops, std::function<Rankwave *(const std::string &)> getStopByName);
};
