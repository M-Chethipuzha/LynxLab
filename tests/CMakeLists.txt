cmake_minimum_required(VERSION 3.20)

# Codec unit tests
add_executable(test_codec test_codec.c)
target_link_libraries(test_codec PRIVATE lynx-codec)
target_include_directories(test_codec PRIVATE ${CMAKE_SOURCE_DIR}/src/codec)
add_test(NAME test_codec COMMAND test_codec)
