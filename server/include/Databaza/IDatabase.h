#pragma once
#include <string>
#include <optional>
#include <vector>
#include <unordered_map>

struct EventFilter {
    std::optional<std::string> from;
    std::optional<std::string> to;
    std::optional<int> limit;
    std::optional<std::string> status;
    std::optional<std::string> source_service;
};

struct EventRecord {
    long long id;
    std::string source_service;
    std::string timestamp_utc;
    std::string payload;
    std::string received_at;
    std::string status;
    int schema_version;
    bool processed;
};

struct Stats {
    int total = 0;
    int success = 0;
    int error = 0;
    std::unordered_map<std::string, int> by_source;
};


class IDatabase {
public:
    virtual ~IDatabase() = default;
    virtual bool init() = 0;
    virtual bool saveMessage(const std::string& source,
        const std::string& timestamp,
        const std::string& status,
        const std::string& payload,
        int schema_version,
        long long& msgId) = 0;
    virtual void close() = 0;
    virtual Stats getStats() const = 0;
    virtual bool markProcessed(long long msgId) = 0;
    virtual std::vector<EventRecord> getEvents(const EventFilter& filter) const = 0;
};