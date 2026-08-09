find_package(GTest REQUIRED)

add_subdirectory(fakes)
add_subdirectory(preview)

add_executable(antwika_app_tests
    AssetPathTest.cpp
    ConsoleLoggingTest.cpp
    FramePacedSourceTest.cpp
    FramePacingTraceTest.cpp
    FramePresentationTest.cpp
    FramePreviewTest.cpp
    FullscreenToggleSourceTest.cpp
    HoverRecordingTest.cpp
    MaxTicksTest.cpp
    PngFileTest.cpp
    PointerReadingTest.cpp
    RunGuardedTest.cpp
    RunRecordedTest.cpp
    TickLimitSourceTest.cpp
    WavFileTest.cpp
    WindowCloseSourceTest.cpp
    WindowEventsTest.cpp
    WindowInputSourceTest.cpp
    WindowPointerMappingTest.cpp
    WindowedHostTest.cpp
    WindowedSessionTest.cpp
)

target_link_libraries(antwika_app_tests
    PRIVATE
        antwika::app
        antwika::app::tests::fakes
        antwika::gfx::tests::mocks
        antwika::input::tests::fakes
        antwika::log::tests::mocks
        antwika::testing
        antwika::time::tests::fakes
        gtest::gtest
)

antwika_bundle_test(TARGET antwika_app_tests)
