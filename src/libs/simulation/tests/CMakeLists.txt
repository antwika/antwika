find_package(GTest REQUIRED)

# antwika::replay is linked by the tests and not by the library: a test
# of the loop needs some ITickSource to script, and ReplaySource is the
# one every application already uses for that.
# The dependency runs the same way it does everywhere else -- these
# tests link the format, the library under test does not.
add_executable(antwika_simulation_tests
    EngineLoopTest.cpp
    TickPacerTest.cpp
    WindowInputSourceTest.cpp
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

include(GoogleTest)
gtest_discover_tests(antwika_simulation_tests)
