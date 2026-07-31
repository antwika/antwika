find_package(GTest REQUIRED)

add_subdirectory(fakes)

add_executable(antwika_app_tests
    ConsoleLoggingTest.cpp
    FramePacedSourceTest.cpp
    PngFileTest.cpp
    PointerReadingTest.cpp
    RunGuardedTest.cpp
    RunRecordedTest.cpp
    WavFileTest.cpp
)

target_link_libraries(antwika_app_tests
    PRIVATE
        antwika::app
        antwika::app::tests::fakes
        antwika::time::tests::fakes
        gtest::gtest
)

include(GoogleTest)
gtest_discover_tests(antwika_app_tests)
