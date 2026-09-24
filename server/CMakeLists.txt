cmake_minimum_required(VERSION 3.20)

add_library(receiver_lib STATIC
    src/Message.cpp
    src/MessageHandler.cpp
    src/ControlService.cpp
    src/GetEventsCommand.cpp
    src/Databaza/SqliteDatabase.cpp
)
target_include_directories(receiver_lib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
target_link_libraries(receiver_lib PUBLIC
    nlohmann_json::nlohmann_json
    SQLite::SQLite3
)

add_executable(receiver
    src/Receiver.cpp
)
target_link_libraries(receiver PRIVATE
    receiver_lib
    net_common
)

add_custom_command(TARGET receiver POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory
            "$<TARGET_FILE_DIR:receiver>/db"
)

enable_testing()
add_subdirectory(../tests tests)