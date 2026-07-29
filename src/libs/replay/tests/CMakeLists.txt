find_package(GTest REQUIRED)

add_executable(antwika_replay_tests
    EngineLoopTest.cpp
    EventJsonTest.cpp
    ReplayCliTest.cpp
    ReplayDeterminismTest.cpp
    ReplayJsonTest.cpp
    ReplaySourceTest.cpp
    ReplayWriterReaderTest.cpp
)

target_include_directories(antwika_replay_tests
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../src
)

target_link_libraries(antwika_replay_tests
    PRIVATE
        antwika::replay
        antwika::engine::tests::mocks
        antwika::event::tests::mocks
        antwika::log::tests::mocks
        gtest::gtest
)

include(GoogleTest)
gtest_discover_tests(antwika_replay_tests)
