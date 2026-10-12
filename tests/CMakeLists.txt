find_package(GTest CONFIG REQUIRED)

add_executable(unit_tests
    test_calculator.cpp
    test_message.cpp
    test_control_service.cpp
    test_sqlite_database.cpp
)

target_link_libraries(unit_tests PRIVATE
    calculator_lib
    receiver_lib
    GTest::gtest
    GTest::gtest_main
    nlohmann_json::nlohmann_json
    SQLite::SQLite3
)

include(GoogleTest)
gtest_discover_tests(unit_tests)