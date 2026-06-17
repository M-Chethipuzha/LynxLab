cmake_minimum_required(VERSION 3.20)

add_executable(lynx-agent
    main.c
    lynx_agent.c
)

target_link_libraries(lynx-agent PRIVATE lynx-codec)

target_include_directories(lynx-agent PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/src/codec
)

install(TARGETS lynx-agent RUNTIME DESTINATION bin)
