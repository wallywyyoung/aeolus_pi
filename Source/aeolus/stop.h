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

#pragma once

#include "aeolus/globals.h"
#include "aeolus/rankwave.h"
#include "aeolus/utilities/Range.h"

#include <nlohmann/json.hpp>

/**
 * This class represents a single stop.
 * A stop can be a combination of pipes arranged in zones.
 * A zone is defined for a continuous range of keys and is
 * composed of one or multiple pipes (e.g. mixtures).
 */
class Stop
{
public:
    // Stop type.
    enum class Type { Unknown, Principal, Flute, Reed, String };

    // Zone - a grouping pipes for a range of keys.
    struct Zone {
        Range keyRange;
        std::vector<std::shared_ptr<Rankwave>> rankwaves;
        [[nodiscard]] bool isForKey(const int key) const noexcept { return keyRange.contains(key); }
    };

    explicit Stop(const Configuration& config);

    void initFromJson(const nlohmann::json& v);

    [[nodiscard]] Type getType() const noexcept { return _type; }
    void setType(const Type t) noexcept { _type = t; }

    [[nodiscard]] std::string getName() const { return _name; }
    void setName(const std::string& name) { _name = name; }

    [[nodiscard]] float getGain() const noexcept;

    void setGain(float g) noexcept { _gain = g; }

    [[nodiscard]] float getChiffGain() const noexcept { return _chiffGain; }
    void setChiffGain(float g) noexcept { _chiffGain = g; }

    [[nodiscard]] bool isEnabled() const noexcept { return _enabled; }
    void setEnabled(const bool shouldBeEnabled) noexcept { _enabled = shouldBeEnabled; }

    [[nodiscard]] const std::vector<Zone>& getZones() const noexcept { return _zones; }

    /**
     * Add a zone that consists of a single rankwave (pipe)
     * that covers its entire range of keys.
     */
    void addZone(const std::shared_ptr<Rankwave> &ptr);

    /**
     * Add a zone composed of multiple pipes.
     */
    void addZone(const std::vector<std::shared_ptr<Rankwave>> &rw);

    /**
     * Returns the range of keys this stop can be triggered by.
     */
    Range getKeyRange() const;

    // Convert stop type from its string name.
    static Type getTypeFromString(const std::string& n);

private:
    std::vector<std::shared_ptr<Rankwave>> getRankwavesFromPipeVar(const nlohmann::json& v) const;

    Type _type{Type::Unknown};
    std::string _name{};
    std::vector<Zone> _zones{};
    float _gain{1.0f};
    float _chiffGain{0.0f};
    bool _enabled{false};
    const Configuration& configuration;
};


