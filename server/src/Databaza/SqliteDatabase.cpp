#include "Databaza/SqliteDatabase.h"
#include <sstream>

SqliteDatabase::SqliteDatabase(const std::string& path, Logger& log)
    : dbPath_(path), log_(log) {
}

SqliteDatabase::~SqliteDatabase() {
    close();
}

bool SqliteDatabase::init() {
    int rc = sqlite3_open(dbPath_.c_str(), &db_);
    if (rc != SQLITE_OK) {
        log_.error(std::string("Error opening database: ") + sqlite3_errmsg(db_));
        return false;
    }
    const char* createTableSQL = R"(
    CREATE TABLE IF NOT EXISTS messages (
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        source_service TEXT NOT NULL,
        timestamp_utc DATETIME NOT NULL,
        payload TEXT NOT NULL,
        received_at DATETIME DEFAULT CURRENT_TIMESTAMP,
        status TEXT NOT NULL DEFAULT 'error'
            CHECK (status IN ('success', 'error')),
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
        log_.error(std::string("Error creating table: ") + (errMsg ? errMsg : "unknown"));
        sqlite3_free(errMsg);
        return false;
    }
    log_.info("SQLite database initialized");
    return true;
}

bool SqliteDatabase::saveMessage(const std::string& source,
    const std::string& timestamp,
    const std::string& status,
    const std::string& payload,
    int schema_version,
    long long& msgId) {
    std::lock_guard<std::mutex> lock(mutex_);
    const char* insertSQL = R"(
    INSERT INTO messages (source_service, timestamp_utc, status, payload, schema_version)
    VALUES (?, ?, ?, ?, ?)
    )";
    sqlite3_stmt* raw = nullptr;
    if (sqlite3_prepare_v2(db_, insertSQL, -1, &raw, nullptr) != SQLITE_OK) {
        log_.error(std::string("Prepare failed: ") + sqlite3_errmsg(db_));
        return false;
    }
    SqliteStmtPtr stmt(raw);
    sqlite3_bind_text(stmt.get(), 1, source.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt.get(), 2, timestamp.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt.get(), 3, status.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt.get(), 4, payload.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt.get(), 5, schema_version);
    if (sqlite3_step(stmt.get()) != SQLITE_DONE) {
        log_.error(std::string("Insert failed: ") + sqlite3_errmsg(db_));
        return false;
    }
    msgId = sqlite3_last_insert_rowid(db_);
    return true;
}

Stats SqliteDatabase::getStats() const {
    std::lock_guard<std::mutex> lock(mutex_);
    Stats s;
    auto prepare = [this](const char* sql) -> SqliteStmtPtr {
        sqlite3_stmt* raw = nullptr;
        if (sqlite3_prepare_v2(db_, sql, -1, &raw, nullptr) != SQLITE_OK) {
            return SqliteStmtPtr(nullptr);
        }
        return SqliteStmtPtr(raw);
        };
    if (auto stmt = prepare("SELECT COUNT(*) FROM messages")) {
        if (sqlite3_step(stmt.get()) == SQLITE_ROW)
            s.total = sqlite3_column_int(stmt.get(), 0);
    }
    if (auto stmt = prepare("SELECT COUNT(*) FROM messages WHERE status='success'")) {
        if (sqlite3_step(stmt.get()) == SQLITE_ROW)
            s.success = sqlite3_column_int(stmt.get(), 0);
    }
    if (auto stmt = prepare("SELECT COUNT(*) FROM messages WHERE status='error'")) {
        if (sqlite3_step(stmt.get()) == SQLITE_ROW)
            s.error = sqlite3_column_int(stmt.get(), 0);
    }
    if (auto stmt = prepare("SELECT source_service, COUNT(*) FROM messages GROUP BY source_service")) {
        while (sqlite3_step(stmt.get()) == SQLITE_ROW) {
            const char* src = reinterpret_cast<const char*>(sqlite3_column_text(stmt.get(), 0));
            s.by_source[src ? src : "unknown"] = sqlite3_column_int(stmt.get(), 1);
        }
    }
    return s;
}

std::vector<EventRecord> SqliteDatabase::getEvents(const EventFilter& filter) const {
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
    const std::string sqlStr = sql.str();
    sqlite3_stmt* raw = nullptr;
    if (sqlite3_prepare_v2(db_, sqlStr.c_str(), -1, &raw, nullptr) != SQLITE_OK) {
        log_.error(std::string("GetEvents prepare failed: ") + sqlite3_errmsg(db_));
        return results;
    }
    SqliteStmtPtr stmt(raw);
    int idx = 1;
    for (const auto& val : bindValues) {
        sqlite3_bind_text(stmt.get(), idx++, val.c_str(), -1, SQLITE_TRANSIENT);
    }
    if (filter.limit.has_value()) {
        sqlite3_bind_int(stmt.get(), idx++, filter.limit.value());
    }
    int rc;
    while ((rc = sqlite3_step(stmt.get())) == SQLITE_ROW) {
        EventRecord rec;
        rec.id = sqlite3_column_int64(stmt.get(), 0);

        const char* src = reinterpret_cast<const char*>(sqlite3_column_text(stmt.get(), 1));
        rec.source_service = src ? src : "";

        const char* ts = reinterpret_cast<const char*>(sqlite3_column_text(stmt.get(), 2));
        rec.timestamp_utc = ts ? ts : "";

        const char* pl = reinterpret_cast<const char*>(sqlite3_column_text(stmt.get(), 3));
        rec.payload = pl ? pl : "";

        const char* ra = reinterpret_cast<const char*>(sqlite3_column_text(stmt.get(), 4));
        rec.received_at = ra ? ra : "";

        const char* st = reinterpret_cast<const char*>(sqlite3_column_text(stmt.get(), 5));
        rec.status = st ? st : "";

        rec.schema_version = sqlite3_column_int(stmt.get(), 6);
        rec.processed = sqlite3_column_int(stmt.get(), 7) != 0;

        results.push_back(std::move(rec));
    }
    if (rc != SQLITE_DONE) {
        log_.error(std::string("GetEvents step error: ") + sqlite3_errmsg(db_));
    }
    return results;
}

void SqliteDatabase::close() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (db_) {
        sqlite3_close(db_);
        db_ = nullptr;
    }
}

bool SqliteDatabase::markProcessed(long long msgId) {
    std::lock_guard<std::mutex> lock(mutex_);
    sqlite3_stmt* raw = nullptr;
    if (sqlite3_prepare_v2(db_,
        "UPDATE messages SET processed = 1 WHERE id = ?",
        -1, &raw, nullptr) != SQLITE_OK) {
        return false;
    }
    SqliteStmtPtr stmt(raw);
    sqlite3_bind_int64(stmt.get(), 1, msgId);
    return sqlite3_step(stmt.get()) == SQLITE_DONE;
}