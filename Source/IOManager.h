//
// Created by Wally Young on 6/6/25.
//
#pragma once

AEOLUS_NAMESPACE_BEGIN

#include "aeolus/IR.h"

class IOManager {
public:
    void loadOrganDefinition();
    IRs loadIRs();
    std::vector<aeolus::Addsynth> loadPipes();
private:
    std::vector<std::byte> readBinaryFile(std::string path);
    void readIRWav(IR ir, std::string filePath);
};

AEOLUS_NAMESPACE_END
