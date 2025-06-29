//
// Created by Wally Young on 6/6/25.
//
#pragma once

#include "aeolus/Addsynth.h"
#include "aeolus/IR.h"

class IOManager {
public:
    static IRs loadIRs();
    static std::vector<Addsynth> loadPipes();
private:
    const char* DIRECTORY = "./Resources/stops/";
    const char* BINARY_EXTENSION = ".ae0";
    const char* JSON_EXTENSION = ".json";
    static std::vector<std::byte> readBinaryFile(std::string path);
};
