cmake_minimum_required(VERSION 3.20)

find_package(Threads REQUIRED)

# Daemon unit tests
add_executable(test_daemon test_daemon.cpp
    ${CMAKE_SOURCE_DIR}/src/daemon/lynx_daemon.cpp
    ${CMAKE_SOURCE_DIR}/src/daemon/lynx_storage.cpp)
target_link_libraries(test_daemon PRIVATE lynx-codec SQLite::SQLite3)
target_include_directories(test_daemon PRIVATE
    ${CMAKE_SOURCE_DIR}/src/codec
    ${CMAKE_SOURCE_DIR}/src/agent
    ${CMAKE_SOURCE_DIR}/src/daemon
)
target_link_libraries(test_daemon PRIVATE Threads::Threads)
add_test(NAME test_daemon COMMAND test_daemon)

# Codec unit tests
add_executable(test_codec test_codec.c)
target_link_libraries(test_codec PRIVATE lynx-codec)
target_include_directories(test_codec PRIVATE ${CMAKE_SOURCE_DIR}/src/codec)
add_test(NAME test_codec COMMAND test_codec)

# Agent unit tests
add_executable(test_agent test_agent.c ${CMAKE_SOURCE_DIR}/src/agent/lynx_agent.c)
target_link_libraries(test_agent PRIVATE lynx-codec)
target_include_directories(test_agent PRIVATE
    ${CMAKE_SOURCE_DIR}/src/codec
    ${CMAKE_SOURCE_DIR}/src/agent
)
add_test(NAME test_agent COMMAND test_agent)

# Storage unit tests
find_package(SQLite3 REQUIRED)
add_executable(test_storage test_storage.cpp
    ${CMAKE_SOURCE_DIR}/src/daemon/lynx_storage.cpp)
target_link_libraries(test_storage PRIVATE SQLite::SQLite3)
target_include_directories(test_storage PRIVATE ${CMAKE_SOURCE_DIR}/src/daemon)
add_test(NAME test_storage COMMAND test_storage)
