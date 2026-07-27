find_package(GTest REQUIRED)

add_subdirectory(fakes)

add_executable(antwika_time_tests
    FakeClockTest.cpp
    SystemClockTest.cpp
    TickTest.cpp
)

target_link_libraries(antwika_time_tests
    PRIVATE
        antwika::time
        antwika::time::tests::fakes
        gtest::gtest
)

include(GoogleTest)
gtest_discover_tests(antwika_time_tests)
