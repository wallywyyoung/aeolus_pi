//
// Created by Wally Young on 6/6/25.
//
#pragma once

#include "aeolus/IR.h"

class IOManager {
public:
    static void loadOrganDefinition();
    static IRs loadIRs();
    static std::vector<aeolus::Addsynth> loadPipes();
private:
    const char* DIRECTORY = "./Resources/stops/";
    const char* BINARY_EXTENSION = ".ae0";
    const char* JSON_EXTENSION = ".json";
    static std::vector<std::byte> readBinaryFile(std::string path);
};
