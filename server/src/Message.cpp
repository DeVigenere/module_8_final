#include "Message.h"
#include <regex>
#include <set>

namespace {
    const std::set<std::string> kAllowedSources = { "calculator", "ipv4_filter", "control" };
    const std::regex kIso8601(R"(^\d{4}-\d{2}-\d{2}T\d{2}:\d{2}:\d{2}(\.\d{1,3})?Z$)");

    bool validateTypes(const nlohmann::json& j) {
        if (!j["source_service"].is_string()) return false;
        return j["timestamp_utc"].is_string()
            && j["payload"].is_object()
            && (!j.contains("status") || j["status"].is_string())
            && (!j.contains("schema_version") || j["schema_version"].is_number_integer())
            && (!j.contains("processed") || j["processed"].is_boolean());
    }

    bool validateSemantics(const nlohmann::json& j) {
        const auto src = j["source_service"].get<std::string>();
        if (!kAllowedSources.count(src)) return false;
        if (!std::regex_match(j["timestamp_utc"].get<std::string>(), kIso8601)) return false;
        if (j.contains("status")) {
            const auto st = j["status"].get<std::string>();
            if (st != "success" && st != "error") return false;
        }
        return true;
    }
}

bool fillMessage(const nlohmann::json& j, Message& msg) {
    if (!j.contains("source_service") || !j.contains("timestamp_utc") || !j.contains("payload"))
        return false;
    if (!validateTypes(j) || !validateSemantics(j))
        return false;
    msg.source_service = j["source_service"].get<std::string>();
    msg.timestamp_utc = j["timestamp_utc"].get<std::string>();
    msg.payload = j["payload"];
    msg.status = j.value("status", std::string{ "success" });
    if (j.contains("schema_version")) msg.schema_version = j["schema_version"].get<int>();
    if (j.contains("processed"))      msg.processed = j["processed"].get<bool>();
    return true;
}

std::optional<nlohmann::json> parseMessage(const std::string& jsonStr, Message& msg) {
    nlohmann::json j;
    try {
        j = nlohmann::json::parse(jsonStr);
    }
    catch (const nlohmann::json::exception&) {
        return std::nullopt;
    }
    try {
        if (!fillMessage(j, msg)) return std::nullopt;
    }
    catch (const nlohmann::json::exception&) {
        return std::nullopt;
    }
    return j;
}