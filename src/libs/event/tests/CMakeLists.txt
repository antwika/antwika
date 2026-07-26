find_package(GTest REQUIRED)

add_subdirectory(mocks)

add_executable(event_tests
    EventDispatcherTest.cpp
    EventQueueTest.cpp
    EventRecorderTest.cpp
    EventTest.cpp
)

target_link_libraries(event_tests
    PRIVATE
        antwika::event
        event_mocks
        gtest::gtest
)

include(GoogleTest)
gtest_discover_tests(event_tests)
