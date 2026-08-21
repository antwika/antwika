find_package(GTest REQUIRED)

add_subdirectory(mocks)

add_executable(antwika_engine_tests
    EngineLoopTest.cpp
    EngineTest.cpp
    EventsTest.cpp
    StopSignalTest.cpp
)

target_link_libraries(antwika_engine_tests
    PRIVATE
        antwika::engine
        antwika::engine::tests::mocks
        antwika::replay
        antwika::log::tests::mocks
        antwika::event::tests::mocks
        gtest::gtest
)

antwika_bundle_test(TARGET antwika_engine_tests)
