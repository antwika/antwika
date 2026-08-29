find_package(GTest REQUIRED)

add_subdirectory(fakes)

add_executable(antwika_replay_tests
    EventJsonTest.cpp
    ReplayCliTest.cpp
    ReplayDeterminismTest.cpp
    ReplayDocumentTest.cpp
    ReplayHeaderTest.cpp
    ReplayJsonTest.cpp
    ReplayMigrationsTest.cpp
    ReplayRecorderTest.cpp
    ReplaySourceTest.cpp
    ReplayVersioningTest.cpp
    ReplayWriterReaderTest.cpp
    TickEventSourceConformanceTest.cpp
)

target_include_directories(antwika_replay_tests
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../src
)

target_link_libraries(antwika_replay_tests
    PRIVATE
        antwika::replay
        antwika::replay::tests::fakes
        antwika::engine
        antwika::event::tests::conformance
        antwika::log::tests::mocks
        antwika::testing
        gtest::gtest
)

antwika_bundle_test(TARGET antwika_replay_tests)
