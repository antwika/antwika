find_package(GTest REQUIRED)

add_subdirectory(mocks)

add_executable(antwika_event_tests
    EventDispatcherTest.cpp
    EventTest.cpp
    ITickEventSourceTest.cpp
    TickEventRecorderTest.cpp
    TickEventTest.cpp
    TickedEventDispatcherTest.cpp
)

target_link_libraries(antwika_event_tests
    PRIVATE
        antwika::event
        antwika::event::tests::mocks
        gtest::gtest
)

antwika_bundle_test(TARGET antwika_event_tests)
