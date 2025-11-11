//
// Created by Wally Young on 10/27/25.
//

#include "RestManager.h"
#include <filesystem>
#include <httplib.h>
#include <nlohmann/json.hpp>


void RestManager::runServer(std::shared_ptr<Organ> organ) {
    httplib::Server server;

    server.set_default_headers({
        {"Access-Control-Allow-Origin", "*"},
        {"Access-Control-Allow-Methods", "GET, POST, OPTIONS"},
        {"Access-Control-Allow-Headers", "Content-Type"}
    });

    // Handle OPTIONS preflight requests
    server.Options(".*", [](const httplib::Request& req, httplib::Response& res) {
        res.status = 200;
    });
    server.Get("/config", [](const httplib::Request& req, httplib::Response& res) {
        const std::filesystem::path configFile = "./Resources/configs/default_organ.json";
        std::ifstream stream(configFile);
        auto config = std::string(std::istreambuf_iterator(stream), std::istreambuf_iterator<char>());
        res.set_content(config, "application/json");
    });

    server.Post("/stop", [&](const httplib::Request & req, httplib::Response & res) {
        auto payload = nlohmann::json::parse(req.body);
        auto id = payload.at("id").get<int>();
        auto divisionID = payload.at("division_id").get<int>();
        auto on = payload.at("on").get<bool>();
        if (on) {
            organ->setDivisionStopOn(divisionID, id);
        } else {
            organ->setDivisionStopOff(divisionID, id);
        }
        res.set_content("Command processed successfully.", "text/plain");
    });

    server.Post("/coupler", [&](const httplib::Request & req, httplib::Response & res) {
        auto payload = nlohmann::json::parse(req.body);
        auto id = payload.at("id").get<int>();
        auto divisionID = payload.at("division_id").get<int>();
        auto on = payload.at("on").get<bool>();
        if (on) {
            organ->setDivisionCouplerOn(divisionID, id);
        } else {
            organ->setDivisionCouplerOff(divisionID, id);
        }
        res.set_content("Command processed successfully.", "text/plain");
    });

    server.Post("/tremulant", [&](const httplib::Request & req, httplib::Response & res) {
        auto payload = nlohmann::json::parse(req.body);
        auto divisionID = payload.at("division_id").get<int>();
        auto on = payload.at("on").get<bool>();
        if (on) {
            organ->setDivisionTremulantOn(divisionID);
        } else {
            organ->setDivisionTremulantOff(divisionID);
        }
        res.set_content("Command processed successfully.", "text/plain");
    });

    server.Post("/swell", [&](const httplib::Request & req, httplib::Response & res) {
        auto payload = nlohmann::json::parse(req.body);
        auto divisionID = payload.at("division_id").get<int>();
        auto value = payload.at("value").get<float>();
        organ->handleDivisionSwell(divisionID, value);
        res.set_content("Command processed successfully.", "text/plain");
    });

    server.Post("/setDivisionPiston", [&](const httplib::Request & req, httplib::Response & res) {
        auto payload = nlohmann::json::parse(req.body);
        auto id = payload.at("id").get<int>();
        auto divisionID = payload.at("division_id").get<int>();
        organ->setDivisionPiston(divisionID, id);
        res.set_content("Command processed successfully.", "text/plain");
    });

    server.Post("/recallDivisionPiston", [&](const httplib::Request & req, httplib::Response & res) {
        auto payload = nlohmann::json::parse(req.body);
        auto id = payload.at("id").get<int>();
        auto divisionID = payload.at("division_id").get<int>();
        organ->recallDivisionPiston(divisionID, id);
        res.set_content("Command processed successfully.", "text/plain");
    });

    server.Post("/setGlobalPiston", [&](const httplib::Request & req, httplib::Response & res) {
        auto payload = nlohmann::json::parse(req.body);
        auto id = payload.at("id").get<int>();
        organ->setGlobalPiston(id);
        res.set_content("Command processed successfully.", "text/plain");
    });

    server.Post("/recallGlobalPiston", [&](const httplib::Request & req, httplib::Response & res) {
        auto payload = nlohmann::json::parse(req.body);
        auto id = payload.at("id").get<int>();
        organ->recallGlobalPiston(id);
        res.set_content("Command processed successfully.", "text/plain");
    });

    server.listen("0.0.0.0", 9021);
}