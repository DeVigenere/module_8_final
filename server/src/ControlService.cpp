#include "ControlService.h"
#include <nlohmann/json.hpp>

ControlService::ControlService(Logger& log) : log_(log) {}

void ControlService::registerCommand(std::unique_ptr<IControlCommand> cmd) {
    commands_[cmd->name()] = std::move(cmd);
}

std::string ControlService::handle(const std::string& rawRequest) {
    nlohmann::json j;
    try {
        j = nlohmann::json::parse(rawRequest);
    }
    catch (const std::exception& e) {
        log_.warn(std::string("Control parse error: ") + e.what());
        nlohmann::json err;
        err["status"] = "error";
        err["error_message"] = "Invalid JSON";
        return err.dump();
    }

    std::string command = j["payload"].value("command", "");
    auto it = commands_.find(command);
    if (it == commands_.end()) {
        nlohmann::json err;
        err["status"] = "error";
        err["error_message"] = "Unknown control command: " + command;
        return err.dump();
    }
    return it->second->execute(j["payload"]).dump();
}