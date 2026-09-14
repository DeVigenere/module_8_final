cmake_minimum_required(VERSION 3.20)

add_executable(calculator_sender
    src/Calculator_sender.cpp
    src/Calculator.cpp
)

# Локальные заголовки клиента (Calculator.h)
target_include_directories(calculator_sender PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(calculator_sender PRIVATE
    net_common
    nlohmann_json::nlohmann_json
)