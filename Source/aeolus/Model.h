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

#include "aeolus/AddSynth.h"

/**
 * @brief A collection of all available stops.
 *
 * This class holds a collection of all available stops.
 * These stops models are shared among all the plugin instances.
 */

class Model {
public:
    explicit Model();
    ~Model();
    Model(const Model&) = delete;
    Model& operator=(const Model&) = delete;
    Model(Model&&) = delete;
    Model& operator=(Model&&) = delete;
    [[nodiscard]] std::vector<std::string> getStopNames() const;
    [[nodiscard]] size_t getStopsCount() const { return synths.size(); }
    AddSynth operator[](const int idx) { return synths[idx]; }
    AddSynth operator[](const int idx) const { return synths[idx]; }
private:
    std::vector<AddSynth> synths{};
};


