find_package(GTest REQUIRED)

add_executable(time_tests
    FakeClockTest.cpp
    SystemClockTest.cpp
)

target_link_libraries(time_tests
    PRIVATE
        antwika::time
        time_fakes
        gtest::gtest
)

include(GoogleTest)
gtest_discover_tests(time_tests)
