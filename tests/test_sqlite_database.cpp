#include <gtest/gtest.h>
#include "Databaza/SqliteDatabase.h"
#include "Logger.h"

class NullLogger : public Logger {
public:
    void debug(const std::string&) override {}
    void info(const std::string&) override {}
    void warn(const std::string&) override {}
    void error(const std::string&) override {}
};

class SqliteDatabaseTest : public ::testing::Test {
protected:
    NullLogger log;
    std::unique_ptr<SqliteDatabase> db;

    void SetUp() override {
        db = std::make_unique<SqliteDatabase>(":memory:", log);
        ASSERT_TRUE(db->init());
    }
    void TearDown() override {
        db->close();
    }
};

TEST_F(SqliteDatabaseTest, SaveAndStats) {
    long long id = 0;
    ASSERT_TRUE(db->saveMessage("calculator",
        "2025-01-01T12:00:00Z", "success",
        R"({"a":1,"b":2,"result":3})", 1, id));
    EXPECT_GT(id, 0);

    ASSERT_TRUE(db->saveMessage("calculator",
        "2025-01-02T12:00:00Z", "error",
        R"({"error":"div by zero"})", 1, id));

    Stats s = db->getStats();
    EXPECT_EQ(s.total, 2);
    EXPECT_EQ(s.success, 1);
    EXPECT_EQ(s.error, 1);
    EXPECT_EQ(s.by_source.at("calculator"), 2);
}

TEST_F(SqliteDatabaseTest, MarkProcessed) {
    long long id = 0;
    ASSERT_TRUE(db->saveMessage("calculator",
        "2025-01-01T12:00:00Z", "success", "{}", 1, id));
    EXPECT_TRUE(db->markProcessed(id));
}

TEST_F(SqliteDatabaseTest, GetEventsFilterByStatus) {
    long long id = 0;
    db->saveMessage("calculator", "2025-01-01T12:00:00Z", "success", "{}", 1, id);
    db->saveMessage("calculator", "2025-01-02T12:00:00Z", "error", "{}", 1, id);

    EventFilter f;
    f.status = "error";
    auto events = db->getEvents(f);
    ASSERT_EQ(events.size(), 1u);
    EXPECT_EQ(events[0].status, "error");
}

TEST_F(SqliteDatabaseTest, GetEventsLimit) {
    long long id = 0;
    for (int i = 0; i < 5; ++i) {
        db->saveMessage("calculator",
            "2025-01-0" + std::to_string(i + 1) + "T12:00:00Z",
            "success", "{}", 1, id);
    }
    EventFilter f;
    f.limit = 3;
    EXPECT_EQ(db->getEvents(f).size(), 3u);
}