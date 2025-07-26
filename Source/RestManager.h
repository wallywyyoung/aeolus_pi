//
// Created by Wally Young on 7/24/25.
//

#pragma once

#include <crow.h>
#include <nlohmann/json.hpp>

#include "aeolus/EngineGlobal.h"

class RestManager {
crow::SimpleApp app;
    RestManager() {
        CROW_ROUTE(app, "/stops")([]() {
        });
        CROW_ROUTE(app, "/stop/<int>/toggle")([](){
            return "Hello world";
        });
    }
};
