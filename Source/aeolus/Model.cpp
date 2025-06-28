//
// Created by Wally Young on 6/27/25.
//

#include "Model.h"
#include "../IOManager.h"

using namespace aeolus;

Model::Model() {
    synths = IOManager::loadPipes();
}

std::vector<std::string> Model::getStopNames() const
{
    std::vector<std::string> stops;

    for (const auto synth : synths) {
        stops.push_back(synth.getStopName());
    }

    return stops;
}

Model::~Model() {
    synths.clear();
}
