find_package(GTest REQUIRED)

add_subdirectory(fakes)

add_executable(antwika_app_tests
    AppRuntimeTest.cpp
    ConsoleLoggingTest.cpp
    FramePacedSourceTest.cpp
    FramePacingStatsTest.cpp
    FramePresentationTest.cpp
    FullscreenToggleSourceTest.cpp
    HoverRecordingTest.cpp
    ParseMaxTicksTest.cpp
    PointerReadingTest.cpp
    RunCatchingErrorsTest.cpp
    RunRecordedTest.cpp
    SpawnDetachedTest.cpp
    TickDebtTest.cpp
    TickEventSourceConformanceTest.cpp
    TickLimitSourceTest.cpp
    WavFileTest.cpp
    WindowCloseSourceTest.cpp
    WindowEventsTest.cpp
    WindowInputSourceTest.cpp
    WindowPointerMappingTest.cpp
    WindowedSessionTest.cpp
)

target_link_libraries(antwika_app_tests
    PRIVATE
        antwika::app
        antwika::app::tests::fakes
        antwika::event::tests::conformance
        antwika::event::tests::fakes
        antwika::gfx
        antwika::gfx::tests::mocks
        antwika::input::tests::fakes
        antwika::log::tests::mocks
        antwika::testing
        antwika::time::tests::fakes
        gtest::gtest
)

antwika_bundle_test(TARGET antwika_app_tests)
