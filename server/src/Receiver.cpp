#include "MessageHandler.h"
#include "ControlService.h"
#include "ConsoleLogger.h"
#include "Databaza/SqliteDatabase.h"
#include "Network/NetworkFactory.h"
#include "Network/IConnection.h"
#include "Network/INetworkFactory.h"
#include "Network/IListener.h"
#include "StatsCommand.h"
#include "GetEventsCommand.h"
#include <atomic>
#include <thread>
#include <memory>

const int PORT = 8080;
const int BUFFER = 1024;
std::atomic<bool> running{ true };

void handleClient(std::unique_ptr<IConnection> client, MessageHandler& handler) {
    if (!client) {
        return;
    }
    std::string received;
    while (running && client->isOpen()) {
        std::string chunk;
        if (!client->recvSome(chunk, BUFFER)) {
            if (!client->isOpen()) {
                break;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }
        received += chunk;
        size_t pos;
        while ((pos = received.find('\n')) != std::string::npos) {
            std::string message = received.substr(0, pos);
            received.erase(0, pos + 1);
            if (!message.empty()) {
                std::string response = handler.handle(message);
                client->sendAll(response);
            }
        }
    }
    client->close();
}

int main() {
    ConsoleLogger log;
    SqliteDatabase db("db/messages.db", log);
    if (!db.init()) {
        return 1;
    }
    ControlService control(log);
    MessageHandler handler(db, control, log);
    control.registerCommand(std::make_unique<StatsCommand>(db, log));
    control.registerCommand(std::make_unique<GetEventsCommand>(db, log));
    auto factory = makeNetworkFactory();
    if (!factory || !factory->init()) {
        log.error("Failed to init network");
        return 1;
    }
    auto listener = factory->listen(PORT);
    if (!listener) {
        log.error("Failed to listen on port " + std::to_string(PORT));
        factory->cleanup();
        return 1;
    }
    log.info("Service on port " + std::to_string(PORT));
    while (running) {
        auto client = listener->accept();
        if (!client) {
            if (running) {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
            continue;
        }
        std::thread([client = std::move(client), &handler]() mutable {
            handleClient(std::move(client), handler);
            }).detach();
    }
    listener->close();
    factory->cleanup();
    log.info("Service stop");
    return 0;
}