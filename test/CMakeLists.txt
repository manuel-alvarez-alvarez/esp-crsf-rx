cmake_minimum_required(VERSION 3.14)
project(crsf_rx_tests C)

include(FetchContent)
FetchContent_Declare(
        unity
        GIT_REPOSITORY https://github.com/ThrowTheSwitch/Unity.git
        GIT_TAG        v2.6.0
)
FetchContent_MakeAvailable(unity)

add_executable(crsf_tests
        test_main.c
        test_parser_fsm.c
        test_unpack_channels.c
        test_unpack_telemetry.c
        ${CMAKE_CURRENT_SOURCE_DIR}/../src/crsf_parser.c
)

target_include_directories(crsf_tests PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include    # Stub esp_attr.h (must be first)
        ${CMAKE_CURRENT_SOURCE_DIR}/../include # crsf_protocol.h
        ${CMAKE_CURRENT_SOURCE_DIR}/../src     # crsf_parser.h
)

target_link_libraries(crsf_tests PRIVATE unity)

enable_testing()

# Auto-discover individual test cases from RUN_FILTERED_TEST() calls
file(STRINGS ${CMAKE_CURRENT_SOURCE_DIR}/test_main.c _test_lines REGEX "RUN_FILTERED_TEST\\(test_")
foreach(_line ${_test_lines})
    string(REGEX MATCH "RUN_FILTERED_TEST\\(([a-zA-Z_0-9]+)\\)" _ ${_line})
    add_test(NAME ${CMAKE_MATCH_1} COMMAND crsf_tests -n ${CMAKE_MATCH_1})
endforeach()
