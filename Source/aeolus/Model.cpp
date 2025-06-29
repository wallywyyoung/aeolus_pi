//
// Created by Wally Young on 6/27/25.
//

#include "IOManager.h"
#include "aeolus/Model.h"



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
