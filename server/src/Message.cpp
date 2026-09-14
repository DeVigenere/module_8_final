#include "Message.h"
#include "Globals.h"
#include <nlohmann/json.hpp>

bool parseMessage(const std::string& jsonStr, Message& msg) {
    try {
        json j = json::parse(jsonStr);
        if (!j.contains("source_service") || !j.contains("timestamp_utc") || !j.contains("payload")) {
            return false;
        }
        msg.source_service = j["source_service"].get<std::string>();
        msg.timestamp_utc = j["timestamp_utc"].get<std::string>();
        msg.payload = j["payload"].get<std::string>();
        if (j.contains("status")) {
            msg.status = j["status"].get<std::string>();
        }
        if (j.contains("schema_version")) {
            msg.schema_version = j["schema_version"].get<int>();
        }
        if (j.contains("processed")) {
            msg.processed = j["processed"].get<bool>();
        }
        return true;
    }
    catch (const json::parse_error& e) {
        std::cerr << "JSON parse error: " << e.what() << std::endl;
        return false;
    }
}


std::string formatEventsAsJson(const std::vector<EventRecord>& events) {
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
    return j.dump();
}
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
std::string handleControlCommand(const nlohmann::json& j) {
    std::string command = j.value("payload", "");
    if (command == "stats") {
        g_db->showStats();
        nlohmann::json resp;
        resp["status"] = "success";
        resp["message"] = "Stats printed to server console";
        return resp.dump();
    }
    if (command == "get_events") {
        EventFilter filter = parseEventFilter(j);
        auto events = g_db->getEvents(filter);
        std::cout << "GetEvents returned " << events.size() << " records" << std::endl;
        return formatEventsAsJson(events);
    }
    nlohmann::json err;
    err["status"] = "error";
    err["error_message"] = "Unknown control command: " + command;
    return err.dump();
}

void printMessage(const std::string& messageStr) {
    std::cout << "Received: " << messageStr << std::endl;
    Message msg;
    if (!parseMessage(messageStr, msg)) {
        std::cout << "Invalid message format" << std::endl;
        return;
    }
    std::cout << "Source: " << msg.source_service << std::endl;
    std::cout << "Time: " << msg.timestamp_utc << std::endl;
    std::cout << "Data: " << msg.payload << std::endl;
    if (msg.source_service == "control") {
        try {
            nlohmann::json j = nlohmann::json::parse(messageStr);
            std::string response = handleControlCommand(j);
            std::cout << "Control response: " << response << std::endl;
        }
        catch (const std::exception& e) {
            std::cerr << "Control error: " << e.what() << std::endl;
        }
        return;
    }

    long long msgId = 0;
    if (g_db->saveMessage(msg.source_service, msg.timestamp_utc, msg.status, msg.payload, msgId)) {
        std::cout << "Message saved with id=" << msgId << std::endl;
    }
    else {
        std::cerr << "Error saving message" << std::endl;
    }
}