#include "MessageHandler.h"
#include "Message.h"
#include <nlohmann/json.hpp>

MessageHandler::MessageHandler(IDatabase& db, ControlService& control, Logger& log)
    : db_(db), control_(control), log_(log) {
}

std::string MessageHandler::handle(const std::string& rawMessage) {
    log_.info("Received: " + rawMessage);
    Message msg;
    auto parsed = parseMessage(rawMessage, msg);
    if (!parsed) {
        log_.warn("Invalid message format");
        return "ERROR\n";
    }
    log_.info("Source: " + msg.source_service);
    log_.info("Time: " + msg.timestamp_utc);
    log_.info("Data: " + msg.payload.dump());
    if (msg.source_service == "control") {
        return control_.handle(parsed->dump()) + "\n";
    }
    int version = msg.schema_version.value_or(1);
    long long msgId = 0;
    if (db_.saveMessage(msg.source_service, msg.timestamp_utc,
        msg.status, msg.payload.dump(), version, msgId)) {
        db_.markProcessed(msgId);
        log_.info("Message saved with id=" + std::to_string(msgId));
    }
    else {
        log_.error("Error saving message");
    }
    return "OK\n";
}