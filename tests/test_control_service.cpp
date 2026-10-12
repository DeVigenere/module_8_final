#include <gtest/gtest.h>
#include "ControlService.h"
#include "IControlCommand.h"
#include <nlohmann/json.hpp>

class NullLogger : public Logger {
public:
    void debug(const std::string&) override {}
    void info(const std::string&) override {}
    void warn(const std::string&) override {}
    void error(const std::string&) override {}
};

class EchoCommand : public IControlCommand {
public:
    std::string name() const override { return "echo"; }
    nlohmann::json execute(const nlohmann::json& req) override {
        nlohmann::json r;
        r["status"] = "success";
        r["echo"] = req;
        return r;
    }
};

TEST(ControlServiceTest, DispatchesRegisteredCommand) {
    NullLogger log;
    ControlService svc(log);
    svc.registerCommand(std::make_unique<EchoCommand>());

    nlohmann::json req;
    req["source_service"] = "control";
    req["timestamp_utc"] = "2025-01-01T12:00:00Z";
    req["payload"] = { {"command", "echo"}, {"foo", 42} };
    req["status"] = "success";

    std::string resp = svc.handle(req.dump());
    auto j = nlohmann::json::parse(resp);
    EXPECT_EQ(j["status"], "success");
    EXPECT_EQ(j["echo"]["foo"], 42);
}

TEST(ControlServiceTest, UnknownCommand) {
    NullLogger log;
    ControlService svc(log);
    nlohmann::json req;
    req["payload"] = { {"command", "nope"} };
    auto j = nlohmann::json::parse(svc.handle(req.dump()));
    EXPECT_EQ(j["status"], "error");
}

TEST(ControlServiceTest, InvalidJson) {
    NullLogger log;
    ControlService svc(log);
    auto j = nlohmann::json::parse(svc.handle("{not json"));
    EXPECT_EQ(j["status"], "error");
}