cmake_minimum_required(VERSION 3.20)

set(VCPKG_ROOT "${CMAKE_SOURCE_DIR}/vcpkg")
set(VCPKG_EXECUTABLE "${VCPKG_ROOT}/vcpkg")

if(WIN32)
    set(VCPKG_CMD "${VCPKG_EXECUTABLE}.exe")
else()
    set(VCPKG_CMD "${VCPKG_EXECUTABLE}")
endif()

if(NOT EXISTS "${VCPKG_ROOT}")
    execute_process(
        COMMAND git clone --depth=1 https://github.com/Microsoft/vcpkg.git "${VCPKG_ROOT}"
        RESULT_VARIABLE GIT_RESULT
        OUTPUT_VARIABLE GIT_OUTPUT
        ERROR_VARIABLE GIT_ERROR
    )
endif()

if(NOT EXISTS "${VCPKG_CMD}")
    if(WIN32)
        execute_process(
            COMMAND "${VCPKG_ROOT}/bootstrap-vcpkg.bat" -disableMetrics
            WORKING_DIRECTORY "${VCPKG_ROOT}"
            RESULT_VARIABLE BOOTSTRAP_RESULT
            OUTPUT_VARIABLE BOOTSTRAP_OUTPUT
            ERROR_VARIABLE BOOTSTRAP_ERROR
        )
    else()
        execute_process(
            COMMAND "${VCPKG_ROOT}/bootstrap-vcpkg.sh" -disableMetrics
            WORKING_DIRECTORY "${VCPKG_ROOT}"
            RESULT_VARIABLE BOOTSTRAP_RESULT
            OUTPUT_VARIABLE BOOTSTRAP_OUTPUT
            ERROR_VARIABLE BOOTSTRAP_ERROR
        )
    endif()
endif()

execute_process(
    COMMAND "${VCPKG_CMD}" install --x-manifest-root="${CMAKE_SOURCE_DIR}" --x-install-root="${CMAKE_SOURCE_DIR}/vcpkg_installed"
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
    RESULT_VARIABLE VCPKG_RESULT
    OUTPUT_VARIABLE VCPKG_OUTPUT
    ERROR_VARIABLE VCPKG_ERROR
)

set(CMAKE_TOOLCHAIN_FILE "${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" CACHE STRING "" FORCE)

project(module_7 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

find_package(SQLite3 REQUIRED)
find_package(nlohmann_json REQUIRED)

set(COMMON_DIR "${CMAKE_SOURCE_DIR}/common")

add_library(net_common STATIC
    ${COMMON_DIR}/src/Network/NetworkFactory.cpp
)

if(WIN32)
    target_sources(net_common PRIVATE
        ${COMMON_DIR}/src/Network/WinNetworkFactory.cpp
        ${COMMON_DIR}/src/Network/WinConnection.cpp
        ${COMMON_DIR}/src/Network/WinListener.cpp
    )
    target_link_libraries(net_common PUBLIC ws2_32)
else()
    target_sources(net_common PRIVATE
        ${COMMON_DIR}/src/Network/UnixNetworkFactory.cpp
        ${COMMON_DIR}/src/Network/UnixConnection.cpp
        ${COMMON_DIR}/src/Network/UnixListener.cpp
    )
    find_package(Threads REQUIRED)
    target_link_libraries(net_common PUBLIC Threads::Threads)
endif()

target_include_directories(net_common PUBLIC
    ${COMMON_DIR}/include
    ${COMMON_DIR}/include/Network
)

add_subdirectory(client)
add_subdirectory(server)