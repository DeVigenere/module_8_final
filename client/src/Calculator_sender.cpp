#include "Calculator.h"
#include "Network/NetworkFactory.h"
#include "Network/IConnection.h"
#include "Network/INetworkFactory.h"
#include <nlohmann/json.hpp>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <chrono>
#include <thread>
#include <memory>

using json = nlohmann::json;

const int PORT = 8080;

std::string buildMessage(const std::string& source,
    const std::string& status,
    const nlohmann::json& payload) {
    auto now = std::chrono::system_clock::now();
    auto now_c = std::chrono::system_clock::to_time_t(now);
    auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;
    std::tm tm_buf;
#ifdef _WIN32
    gmtime_s(&tm_buf, &now_c);
#else
    gmtime_r(&now_c, &tm_buf);
#endif
    std::stringstream ss;
    ss << std::put_time(&tm_buf, "%Y-%m-%dT%H:%M:%S")
        << "." << std::setfill('0') << std::setw(3) << now_ms.count() << "Z";
    json messageJson;
    messageJson["source_service"] = source;
    messageJson["timestamp_utc"] = ss.str();
    messageJson["payload"] = payload;
    messageJson["status"] = status;
    messageJson["schema_version"] = 1;
    return messageJson.dump() + "\n";
}

bool sendMessage(const std::string& message) {
    static auto factory = makeNetworkFactory();
    if (!factory || !factory->init()) {
        std::cerr << "Failed to initialize network" << std::endl;
        return false;
    }
    auto connection = factory->connectTo("127.0.0.1", PORT);
    if (!connection) {
        std::cerr << "Failed to connect to server" << std::endl;
        factory->cleanup();
        return false;
    }
    if (!connection->sendAll(message)) {
        std::cerr << "Failed to send message" << std::endl;
        connection->close();
        factory->cleanup();
        return false;
    }
    std::string response;
    if (connection->recvSome(response, 1024)) {
        std::cout << "Server response: " << response << std::endl;
    }
    connection->close();
    factory->cleanup();
    return true;
}

int main() {
    Calculator calc;
    struct Operation {
        int a, b;
        char op;
    };
    Operation ops[] = {
        {10, 5, '+'},
        {20, 4, '-'},
        {7, 3, '*'},
        {15, 3, '/'},
        {8, 0, '/'}
    };
    int sent_count = 0;
    int total_ops = sizeof(ops) / sizeof(ops[0]);
    for (const auto& op : ops) {
        try {
            nlohmann::json payloadJson;
            payloadJson["a"] = op.a;
            payloadJson["b"] = op.b;
            payloadJson["operation"] = std::string(1, op.op);

            switch (op.op) {
            case '+': payloadJson["result"] = calc.Add(op.a, op.b); break;
            case '-': payloadJson["result"] = calc.Subtract(op.a, op.b); break;
            case '*': payloadJson["result"] = calc.Multiply(op.a, op.b); break;
            case '/': payloadJson["result"] = calc.Divide(op.a, op.b); break;
            }

            std::string message = buildMessage("calculator", "success", payloadJson);
            std::cout << "\nSending: " << message;
            if (sendMessage(message)) {
                sent_count++;
            }
            else {
                std::cerr << "Failed send message" << std::endl;
            }
        }
        catch (const std::exception& e) {
            std::cerr << "Calculation error: " << e.what() << std::endl;
            nlohmann::json payloadJson;
            payloadJson["a"] = op.a;
            payloadJson["b"] = op.b;
            payloadJson["operation"] = std::string(1, op.op);
            payloadJson["error"] = e.what();
            std::string message = buildMessage("calculator", "error", payloadJson);
            std::cout << "\nSending error: " << message;
            if (sendMessage(message)) {
                sent_count++;
            }
        }
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    std::cout << "\nSent " << sent_count << " out of " << total_ops << " messages" << std::endl;
    return 0;
}