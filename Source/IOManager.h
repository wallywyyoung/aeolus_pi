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
    static std::vector<std::byte> readBinaryFile(const std::string &path);
    static void addsynthFromJson(const std::filesystem::directory_entry& entry, Addsynth &adsynth);
    static void addsynthFromBinary(const std::filesystem::directory_entry& entry, Addsynth &addsynth);
};
