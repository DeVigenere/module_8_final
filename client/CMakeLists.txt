cmake_minimum_required(VERSION 3.20)

add_library(calculator_lib STATIC
    src/Calculator.cpp
)
target_include_directories(calculator_lib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

add_executable(calculator_sender
    src/Calculator_sender.cpp
)
target_link_libraries(calculator_sender PRIVATE
    calculator_lib
    net_common
    nlohmann_json::nlohmann_json
)