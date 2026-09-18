#pragma once
#include "IDatabase.h"
#include "Logger.h"
#include <sqlite3.h>
#include <mutex>
#include <string>
#include <memory>

class SqliteDatabase : public IDatabase {
    sqlite3* db_ = nullptr;
    mutable std::mutex mutex_;
    std::string dbPath_;
    Logger& log_;
public:
    SqliteDatabase(const std::string& path, Logger& log);
    ~SqliteDatabase() override;
    bool init() override;
    bool saveMessage(const std::string& source,
        const std::string& timestamp,
        const std::string& status,
        const std::string& payload,
        int schema_version,
        long long& msgId) override;
    Stats getStats() const override;
    void close() override;
    bool markProcessed(long long msgId) override;
    std::vector<EventRecord> getEvents(const EventFilter& filter) const override;
};

struct StmtDeleter {
    void operator()(sqlite3_stmt* stmt) const noexcept {
        if (stmt) sqlite3_finalize(stmt);
    }
};
using SqliteStmtPtr = std::unique_ptr<sqlite3_stmt, StmtDeleter>;