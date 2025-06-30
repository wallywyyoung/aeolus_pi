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
    static std::vector<std::byte> readBinaryFile(std::string path);
};
