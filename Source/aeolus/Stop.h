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
// ---------------------------------------------------------------------------

#pragma once

#include "aeolus/RankWave.h"
#include "aeolus/utilities/Range.h"

/**
 * This class represents a single stop.
 * A stop can be a combination of pipes arranged in zones.
 * A zone is defined for a continuous range of keys and is
 * composed of one or multiple pipes (e.g. mixtures).
 */
class Stop {
public:
    enum class Type { Unknown, Principal, Flute, Reed, String };

    // Zone - a grouping pipes for a range of keys.
    struct Zone {
        Range keyRange;
        std::vector<std::shared_ptr<RankWave>> rankWaves;
        [[nodiscard]] bool isForKey(const int key) const noexcept { return keyRange.contains(key); }
    };

    explicit Stop() = default;

    [[nodiscard]] Type getType() const noexcept { return type; }
    [[nodiscard]] std::string getName() const { return name; }
    [[nodiscard]] float getGain() const noexcept { return gain; }
    [[nodiscard]] float getChiffGain() const noexcept { return chiffGain; }
    [[nodiscard]] const std::vector<Zone>& getZones() const noexcept { return zones; }

    [[nodiscard]] bool isEnabled() const noexcept { return enabled; }
    void setEnabled(const bool enable) noexcept { enabled = enable; }


    /**
     * Returns the range of keys this stop can be triggered by.
     */
    [[nodiscard]] Range getKeyRange() const {
        if (zones.empty()) {
            return {};
        }
        auto range(zones[0].keyRange);
        for (const auto&[keyRange, rankWaves] : zones)
            range = range.getUnionWith(keyRange);
        return range;
    }

private:
    Type type{Type::Unknown};
    std::string name{};
    std::vector<Zone> zones{};
    float gain{1.0f};
    float chiffGain{0.0f};
    bool enabled{false};

    friend class StopFactory;
};
