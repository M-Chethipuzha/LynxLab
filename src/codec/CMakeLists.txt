cmake_minimum_required(VERSION 3.20)

# Static library for the wire-protocol codec
add_library(lynx-codec STATIC
    lynx_codec.c
    lynx_codec.h
)

target_include_directories(lynx-codec PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:include/lynx>
)

# Install headers
install(FILES lynx_codec.h DESTINATION include/lynx)