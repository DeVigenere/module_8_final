#pragma once
#include <string>
#include <optional>
#include <iostream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;
std::string handleControlCommand(const nlohmann::json& j);

struct Message {
    std::string source_service;
    std::string timestamp_utc;
    std::string payload;
    std::string status;
    std::optional<int> schema_version;
    std::optional<bool> processed;
};

bool parseMessage(const std::string& jsonStr, Message& msg);
void printMessage(const std::string& messageStr);