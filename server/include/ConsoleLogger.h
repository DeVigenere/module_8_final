#pragma once
#include "Logger.h"
#include <iostream>

class ConsoleLogger : public Logger {
public:
    void debug(const std::string& m) override { std::cout << "[DEBUG] " << m << "\n"; }
    void info(const std::string& m) override { std::cout << "[INFO]  " << m << "\n"; }
    void warn(const std::string& m) override { std::cerr << "[WARN]  " << m << "\n"; }
    void error(const std::string& m) override { std::cerr << "[ERROR] " << m << "\n"; }
};