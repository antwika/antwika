find_package(GTest REQUIRED)

add_subdirectory(fakes)

add_executable(antwika_time_tests
    FakeClockTest.cpp
    FakeSleeperTest.cpp
    SystemClockTest.cpp
    SystemSleeperTest.cpp
    TickTest.cpp
)

target_link_libraries(antwika_time_tests
    PRIVATE
        antwika::time
        antwika::time::tests::fakes
        gtest::gtest
)

antwika_bundle_test(TARGET antwika_time_tests)
