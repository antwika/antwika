find_package(GTest REQUIRED)

add_executable(antwika_replay_tests
    BinaryEventCodecTest.cpp
    BinaryPrimitivesTest.cpp
    BinaryReplayWriterReaderTest.cpp
    EngineLoopTest.cpp
    EventJsonTest.cpp
    JsonReplayWriterReaderTest.cpp
    ReplayDeterminismTest.cpp
    ReplayJsonTest.cpp
    ReplaySourceTest.cpp
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
