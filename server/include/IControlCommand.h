#pragma once
#include <string>
#include <nlohmann/json.hpp>
#include "Databaza/IDatabase.h"
#include "Logger.h"

class IControlCommand {
public:
    virtual ~IControlCommand() = default;
    virtual std::string name() const = 0;
    virtual nlohmann::json execute(const nlohmann::json& request) = 0;
};