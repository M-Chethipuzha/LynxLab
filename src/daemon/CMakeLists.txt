find_package(SQLite3 REQUIRED)

add_executable(lynxd
    main.cpp
    lynx_daemon.cpp
    lynx_storage.cpp
)

target_link_libraries(lynxd PRIVATE lynx-codec SQLite::SQLite3)

target_include_directories(lynxd PRIVATE
    ${CMAKE_SOURCE_DIR}/src/codec
    ${CMAKE_SOURCE_DIR}/src/agent
    ${CMAKE_CURRENT_SOURCE_DIR}
)
