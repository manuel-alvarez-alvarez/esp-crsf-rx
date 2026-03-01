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
add_test(NAME crsf_tests COMMAND crsf_tests)
