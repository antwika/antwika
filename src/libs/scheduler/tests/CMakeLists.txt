find_package(GTest REQUIRED)

add_subdirectory(mocks)

add_executable(antwika_scheduler_tests
    SchedulerDependencyTest.cpp
    SchedulerDeterminismTest.cpp
    SchedulerTest.cpp
)

target_link_libraries(antwika_scheduler_tests
    PRIVATE
        antwika::scheduler
        antwika::scheduler::tests::mocks
        antwika::time
        gtest::gtest
)

include(GoogleTest)
gtest_discover_tests(antwika_scheduler_tests)
