#include "Databaza/SqliteDatabase.h"
#include <iostream>
#include <sstream>

SqliteDatabase::SqliteDatabase(const std::string& path)
    : dbPath_(path) {
}

SqliteDatabase::~SqliteDatabase() {
    close();
}

bool SqliteDatabase::init() {
    int rc = sqlite3_open(dbPath_.c_str(), &db_);
    if (rc != SQLITE_OK) {
        std::cerr << "Error opening database: " << sqlite3_errmsg(db_) << std::endl;
        return false;
    }
    const char* createTableSQL = R"(
        CREATE TABLE IF NOT EXISTS messages (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            source_service TEXT NOT NULL,
            timestamp_utc DATETIME NOT NULL,
            payload TEXT NOT NULL,
            received_at DATETIME DEFAULT CURRENT_TIMESTAMP,
            status TEXT DEFAULT 'received',
            schema_version INTEGER DEFAULT 1,
            processed BOOLEAN DEFAULT 0
        );
        CREATE INDEX IF NOT EXISTS idx_source_service ON messages(source_service);
        CREATE INDEX IF NOT EXISTS idx_timestamp ON messages(timestamp_utc);
        CREATE INDEX IF NOT EXISTS idx_status ON messages(status);
    )";

    char* errMsg = nullptr;
    rc = sqlite3_exec(db_, createTableSQL, nullptr, nullptr, &errMsg);
    if (rc != SQLITE_OK) {
        std::cerr << "Error creating table: " << errMsg << std::endl;
        sqlite3_free(errMsg);
        return false;
    }
    std::cout << "SQLite database initialized" << std::endl;
    return true;
}

bool SqliteDatabase::saveMessage(const std::string& source,
    const std::string& timestamp,
    const std::string& status,
    const std::string& payload,
    long long& msgId) {
    std::lock_guard<std::mutex> lock(mutex_);
    const char* insertSQL = R"(
        INSERT INTO messages (source_service, timestamp_utc, status, payload) VALUES (?, ?, ?, ?)
    )";
    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(db_, insertSQL, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        std::cerr << "Prepare failed: " << sqlite3_errmsg(db_) << std::endl;
        return false;
    }
    sqlite3_bind_text(stmt, 1, source.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, timestamp.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, status.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 4, payload.c_str(), -1, SQLITE_STATIC);
    rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE) {
        std::cerr << "Insert failed: " << sqlite3_errmsg(db_) << std::endl;
        sqlite3_finalize(stmt);
        return false;
    }
    msgId = sqlite3_last_insert_rowid(db_);
    sqlite3_finalize(stmt);
    return true;
}

void SqliteDatabase::showStats() {
    std::lock_guard<std::mutex> lock(mutex_);
    sqlite3_stmt* stmt = nullptr;
    const char* totalSQL = "SELECT COUNT(*) FROM messages";
    if (sqlite3_prepare_v2(db_, totalSQL, -1, &stmt, nullptr) == SQLITE_OK) {
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            int total = sqlite3_column_int(stmt, 0);
            std::cout << "Total messages: " << total << std::endl;
        }
        sqlite3_finalize(stmt);
    }
    const char* successSQL = "SELECT COUNT(*) FROM messages WHERE status = 'success'";
    if (sqlite3_prepare_v2(db_, successSQL, -1, &stmt, nullptr) == SQLITE_OK) {
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            int success = sqlite3_column_int(stmt, 0);
            std::cout << "Success: " << success << std::endl;
        }
        sqlite3_finalize(stmt);
    }
    const char* errorSQL = "SELECT COUNT(*) FROM messages WHERE status = 'error'";
    if (sqlite3_prepare_v2(db_, errorSQL, -1, &stmt, nullptr) == SQLITE_OK) {
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            int error = sqlite3_column_int(stmt, 0);
            std::cout << "Error: " << error << std::endl;
        }
        sqlite3_finalize(stmt);
    }
    const char* sourceSQL = R"(
        SELECT source_service, COUNT(*) 
        FROM messages 
        GROUP BY source_service
    )";
    if (sqlite3_prepare_v2(db_, sourceSQL, -1, &stmt, nullptr) == SQLITE_OK) {
        std::cout << "Messages by source:" << std::endl;
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            const char* source = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
            int count = sqlite3_column_int(stmt, 1);
            std::cout << "  " << (source ? source : "unknown") << ": " << count << std::endl;
        }
        sqlite3_finalize(stmt);
    }
}

std::vector<EventRecord> SqliteDatabase::getEvents(const EventFilter& filter) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<EventRecord> results;
    std::stringstream sql;
    sql << "SELECT id, source_service, timestamp_utc, payload, "
        << "       COALESCE(received_at, ''), "
        << "       COALESCE(status, 'received'), "
        << "       COALESCE(schema_version, 1), "
        << "       COALESCE(processed, 0) "
        << "FROM messages WHERE 1=1";
    std::vector<std::string> bindValues;
    if (filter.from.has_value()) {
        sql << " AND timestamp_utc >= ?";
        bindValues.push_back(filter.from.value());
    }
    if (filter.to.has_value()) {
        sql << " AND timestamp_utc <= ?";
        bindValues.push_back(filter.to.value());
    }
    if (filter.status.has_value()) {
        sql << " AND status = ?";
        bindValues.push_back(filter.status.value());
    }
    if (filter.source_service.has_value()) {
        sql << " AND source_service = ?";
        bindValues.push_back(filter.source_service.value());
    }

    sql << " ORDER BY timestamp_utc DESC";

    if (filter.limit.has_value()) {
        sql << " LIMIT ?";
    }
    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db_, sql.str().c_str(), -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        std::cerr << "GetEvents prepare failed: " << sqlite3_errmsg(db_) << std::endl;
        return results;
    }
    int idx = 1;
    for (const auto& val : bindValues) {
        sqlite3_bind_text(stmt, idx++, val.c_str(), -1, SQLITE_TRANSIENT);
    }
    if (filter.limit.has_value()) {
        sqlite3_bind_int(stmt, idx++, filter.limit.value());
    }
    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        EventRecord rec;
        rec.id = sqlite3_column_int64(stmt, 0);
        const char* src = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        rec.source_service = src ? src : "";

        const char* ts = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        rec.timestamp_utc = ts ? ts : "";

        const char* pl = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        rec.payload = pl ? pl : "";

        const char* ra = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
        rec.received_at = ra ? ra : "";

        const char* st = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));
        rec.status = st ? st : "";

        rec.schema_version = sqlite3_column_int(stmt, 6);
        rec.processed = sqlite3_column_int(stmt, 7) != 0;
        results.push_back(std::move(rec));
    }
    if (rc != SQLITE_DONE) {
        std::cerr << "GetEvents step error: " << sqlite3_errmsg(db_) << std::endl;
    }
    sqlite3_finalize(stmt);
    return results;
}

void SqliteDatabase::close() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (db_) {
        sqlite3_close(db_);
        db_ = nullptr;
    }
}