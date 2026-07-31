find_package(GTest REQUIRED)

add_executable(antwika_app_tests
    ConsoleLoggingTest.cpp
    PngFileTest.cpp
    PointerReadingTest.cpp
    RunGuardedTest.cpp
    RunRecordedTest.cpp
)

target_link_libraries(antwika_app_tests
    PRIVATE
        antwika::app
        gtest::gtest
)

include(GoogleTest)
gtest_discover_tests(antwika_app_tests)
