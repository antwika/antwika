find_package(GTest REQUIRED)

add_executable(antwika_pattern_tests
    CombinatorsTest.cpp
    ControlsTest.cpp
    CycleTest.cpp
    HapBufferTest.cpp
    HapTest.cpp
    ParamValueTest.cpp
    PatternErrorTest.cpp
    PatternTest.cpp
    PatternsTest.cpp
    SpanTest.cpp
)

target_link_libraries(antwika_pattern_tests
    PRIVATE
        antwika::pattern
        gtest::gtest
)

antwika_bundle_test(TARGET antwika_pattern_tests)
