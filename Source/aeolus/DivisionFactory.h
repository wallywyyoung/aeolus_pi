//
// Created by Wally Young on 6/30/25.
//

#pragma once
#include <memory>
#include <nlohmann/json.hpp>
#include "aeolus/Division.h"

class DivisionFactory {
public:
    static std::unique_ptr<Division> initFromJson(nlohmann::json& json, const Engine& engine, const std::string& name = std::string());
};
