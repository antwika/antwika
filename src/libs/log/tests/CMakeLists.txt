find_package(GTest REQUIRED)

add_subdirectory(mocks)

add_executable(antwika_log_tests
    LevelTest.cpp
    LoggerTest.cpp
    MinimumLevelLogPolicyTest.cpp
    NullAppenderTest.cpp
    PlainFormatterTest.cpp
    StreamAppenderTest.cpp
)

target_link_libraries(antwika_log_tests
    PRIVATE
        antwika::log
        antwika::log::tests::mocks
        antwika::time::tests::fakes
        gtest::gtest
)

include(GoogleTest)
gtest_discover_tests(antwika_log_tests)
