#include "GetEventsCommand.h"

namespace {
    EventFilter parseEventFilter(const nlohmann::json& j) {
        EventFilter f;
        if (j.contains("from") && !j["from"].is_null())
            f.from = j["from"].get<std::string>();
        if (j.contains("to") && !j["to"].is_null())
            f.to = j["to"].get<std::string>();
        if (j.contains("limit") && !j["limit"].is_null())
            f.limit = j["limit"].get<int>();
        if (j.contains("status") && !j["status"].is_null())
            f.status = j["status"].get<std::string>();
        if (j.contains("source_service") && !j["source_service"].is_null())
            f.source_service = j["source_service"].get<std::string>();
        return f;
    }

    nlohmann::json formatEvents(const std::vector<EventRecord>& events) {
        nlohmann::json j;
        j["source_service"] = "control";
        j["status"] = "success";
        j["count"] = events.size();
        nlohmann::json arr = nlohmann::json::array();
        for (const auto& e : events) {
            nlohmann::json item;
            item["id"] = e.id;
            item["source_service"] = e.source_service;
            item["timestamp_utc"] = e.timestamp_utc;
            item["payload"] = e.payload;
            item["received_at"] = e.received_at;
            item["status"] = e.status;
            item["schema_version"] = e.schema_version;
            item["processed"] = e.processed;
            arr.push_back(item);
        }
        j["events"] = arr;
        return j;
    }
}

nlohmann::json GetEventsCommand::execute(const nlohmann::json& request) {
    EventFilter filter = parseEventFilter(request);
    auto events = db_.getEvents(filter);
    log_.info("GetEvents returned " + std::to_string(events.size()) + " records");
    return formatEvents(events);
}