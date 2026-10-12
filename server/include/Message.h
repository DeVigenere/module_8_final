#pragma once
#include <string>
#include <optional>
#include <nlohmann/json.hpp>

struct Message {
    std::string source_service;
    std::string timestamp_utc;
    nlohmann::json payload;
    std::string status;
    std::optional<int> schema_version;
    std::optional<bool> processed;
};

std::optional<nlohmann::json> parseMessage(const std::string& jsonStr, Message& msg);