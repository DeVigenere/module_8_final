#pragma once
#include "IControlCommand.h"

class StatsCommand : public IControlCommand {
public:
    StatsCommand(IDatabase& db, Logger& log) : db_(db), log_(log) {}
    std::string name() const override { return "stats"; }
    nlohmann::json execute(const nlohmann::json&) override {
        Stats s = db_.getStats();
        nlohmann::json resp;
        resp["status"] = "success";
        resp["total"] = s.total;
        resp["success"] = s.success;
        resp["error"] = s.error;
        if (!s.by_source.empty()) {
            resp["by_source"] = s.by_source;
        }
        return resp;
    }
private:
    IDatabase& db_;
    Logger& log_;
};