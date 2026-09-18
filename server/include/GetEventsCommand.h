#pragma once
#include "IControlCommand.h"

class GetEventsCommand : public IControlCommand {
public:
    GetEventsCommand(IDatabase& db, Logger& log) : db_(db), log_(log) {}
    std::string name() const override { return "get_events"; }
    nlohmann::json execute(const nlohmann::json& request) override;
private:
    IDatabase& db_;
    Logger& log_;
};