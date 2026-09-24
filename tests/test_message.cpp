#include <gtest/gtest.h>
#include "Message.h"
#include <nlohmann/json.hpp>

namespace {
    nlohmann::json makeValid() {
        return {
            {"source_service", "calculator"},
            {"timestamp_utc", "2025-01-01T12:00:00.123Z"},
            {"payload", {{"a", 1}, {"b", 2}, {"operation", "+"}, {"result", 3}}},
            {"status", "success"},
            {"schema_version", 1}
        };
    }
}

TEST(MessageTest, ValidMessageParses) {
    Message msg;
    auto parsed = parseMessage(makeValid().dump(), msg);
    ASSERT_TRUE(parsed.has_value());
    EXPECT_EQ(msg.source_service, "calculator");
    EXPECT_EQ(msg.timestamp_utc, "2025-01-01T12:00:00.123Z");
    EXPECT_EQ(msg.status, "success");
    ASSERT_TRUE(msg.schema_version.has_value());
    EXPECT_EQ(msg.schema_version.value(), 1);
}

TEST(MessageTest, InvalidJsonReturnsNullopt) {
    Message msg;
    EXPECT_FALSE(parseMessage("{not json", msg).has_value());
}

TEST(MessageTest, MissingRequiredFields) {
    Message msg;
    nlohmann::json j = makeValid();
    j.erase("payload");
    EXPECT_FALSE(parseMessage(j.dump(), msg).has_value());
}

TEST(MessageTest, UnknownSourceRejected) {
    Message msg;
    nlohmann::json j = makeValid();
    j["source_service"] = "hacker";
    EXPECT_FALSE(parseMessage(j.dump(), msg).has_value());
}

TEST(MessageTest, BadTimestampRejected) {
    Message msg;
    nlohmann::json j = makeValid();
    j["timestamp_utc"] = "2025-01-01 12:00";
    EXPECT_FALSE(parseMessage(j.dump(), msg).has_value());
}

TEST(MessageTest, BadStatusRejected) {
    Message msg;
    nlohmann::json j = makeValid();
    j["status"] = "unknown";
    EXPECT_FALSE(parseMessage(j.dump(), msg).has_value());
}

TEST(MessageTest, StatusDefaultsToSuccess) {
    Message msg;
    nlohmann::json j = makeValid();
    j.erase("status");
    auto parsed = parseMessage(j.dump(), msg);
    ASSERT_TRUE(parsed.has_value());
    EXPECT_EQ(msg.status, "success");
}