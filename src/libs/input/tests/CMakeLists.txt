find_package(GTest REQUIRED)

add_subdirectory(conformance)
add_subdirectory(fakes)
add_subdirectory(mocks)

add_executable(antwika_input_tests
    InputCapabilitiesTest.cpp
    InputEventCodecTest.cpp
    InputEventTest.cpp
    KeyModifiersTest.cpp
    KeyTest.cpp
    LiveInputSourceTest.cpp
    MouseButtonTest.cpp
    NullInputBackendConformanceTest.cpp
    NullInputBackendTest.cpp
    PositionTest.cpp
)

target_link_libraries(antwika_input_tests
    PRIVATE
        antwika::input
        antwika::input::tests::conformance
        antwika::input::tests::fakes
        antwika::input::tests::mocks
        antwika::log::tests::mocks
        gtest::gtest
)

include(GoogleTest)
gtest_discover_tests(antwika_input_tests)
