find_package(GTest REQUIRED)

add_subdirectory(fakes)

add_executable(antwika_app_tests
    AssetPathTest.cpp
    ConsoleLoggingTest.cpp
    FramePacedSourceTest.cpp
    FramePresentationTest.cpp
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
    WindowInputSourceTest.cpp
    WindowedSessionTest.cpp
    WindowPointerMappingTest.cpp
)

target_link_libraries(antwika_app_tests
    PRIVATE
        antwika::app
        antwika::app::tests::fakes
        antwika::gfx::tests::mocks
        antwika::input::tests::fakes
        antwika::testing
        antwika::time::tests::fakes
        gtest::gtest
)

antwika_bundle_test(TARGET antwika_app_tests)
