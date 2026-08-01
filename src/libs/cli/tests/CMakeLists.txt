find_package(GTest REQUIRED)

add_executable(antwika_cli_tests
    CommandLineTest.cpp
)

target_include_directories(antwika_cli_tests
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../src
)

target_link_libraries(antwika_cli_tests
    PRIVATE
        antwika::cli
        gtest::gtest
)

antwika_bundle_test(TARGET antwika_cli_tests)
