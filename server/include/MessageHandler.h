#pragma once
#include <string>
#include "Databaza/IDatabase.h"
#include "ControlService.h"
#include "Logger.h"

class MessageHandler {
public:
    MessageHandler(IDatabase& db, ControlService& control, Logger& log);
    std::string handle(const std::string& rawMessage);
private:
    IDatabase& db_;
    ControlService& control_;
    Logger& log_;
};