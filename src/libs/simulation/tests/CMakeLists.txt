find_package(GTest REQUIRED)

add_executable(antwika_simulation_tests
    EngineLoopTest.cpp
    TickPacerTest.cpp
)

target_link_libraries(antwika_simulation_tests
    PRIVATE
        antwika::simulation
        antwika::replay
        antwika::engine::tests::mocks
        antwika::event::tests::mocks
        antwika::gfx::tests::mocks
        antwika::log::tests::mocks
        antwika::time::tests::fakes
        gtest::gtest
)

antwika_bundle_test(TARGET antwika_simulation_tests)
