//
// Created by Wally Young on 6/27/25.
//

#pragma once

#include "aeolus/Addsynth.h"
#include "aeolus/globals.h"

/**
 * @brief A collection of all available stops.
 *
 * This class holds a collection of all available stops.
 * These stops models are shared among all the plugin instances.
 */

class Model {
public:
    Model();
    ~Model();
    Model(const Model&) = delete;
    Model& operator=(const Model&) = delete;
    Model(Model&&) = delete;
    Model& operator=(Model&&) = delete;

    [[nodiscard]] std::vector<std::string> getStopNames() const;

    [[nodiscard]] int getStopsCount() const { return synths.size(); }
    Addsynth operator[](const int idx) { return synths[idx]; }
    Addsynth operator[](const int idx) const { return synths[idx]; }

private:
    std::vector<Addsynth> synths{};
};


