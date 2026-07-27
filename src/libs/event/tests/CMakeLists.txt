find_package(GTest REQUIRED)

add_subdirectory(mocks)

add_executable(antwika_event_tests
    EventDispatcherTest.cpp
    EventQueueTest.cpp
    EventRecorderTest.cpp
    EventTest.cpp
    ReplayRecorderTest.cpp
    TickedEventDispatcherTest.cpp
    TimedEventTest.cpp
)

target_link_libraries(antwika_event_tests
    PRIVATE
        antwika::event
        antwika::event::tests::mocks
        gtest::gtest
)

include(GoogleTest)
gtest_discover_tests(antwika_event_tests)
