cmake_minimum_required(VERSION 3.20)

add_executable(receiver
    src/Receiver.cpp
    src/Message.cpp
    src/Globals.cpp
    src/Databaza/SqliteDatabase.cpp
)

target_include_directories(receiver PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(receiver PRIVATE
    net_common
    nlohmann_json::nlohmann_json
    SQLite::SQLite3
)

add_custom_command(TARGET receiver POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory
            "$<TARGET_FILE_DIR:receiver>/db"
    COMMENT "Ensuring db/ directory exists for messages.db"
)