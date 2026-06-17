cmake_minimum_required(VERSION 3.20)

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
