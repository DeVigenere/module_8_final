#pragma once
#include <string>
#include <memory>
#include <unordered_map>
#include "IControlCommand.h"
#include "Logger.h"

class ControlService {
public:
    ControlService(Logger& log);
    void registerCommand(std::unique_ptr<IControlCommand> cmd);
    std::string handle(const std::string& rawRequest);
private:
    Logger& log_;
    std::unordered_map<std::string, std::unique_ptr<IControlCommand>> commands_;
};