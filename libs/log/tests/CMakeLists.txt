find_package(GTest REQUIRED)

add_executable(log_tests
    LoggerTest.cpp
    StreamAppenderTest.cpp
)

target_link_libraries(log_tests
    PRIVATE
        antwika::log
        log_mocks
        time_fakes
        gtest::gtest
)

include(GoogleTest)
gtest_discover_tests(log_tests)
