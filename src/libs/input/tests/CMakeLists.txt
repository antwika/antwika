find_package(GTest REQUIRED)

add_executable(antwika_input_tests
    InputCapabilitiesTest.cpp
    InputEventTest.cpp
    KeyModifiersTest.cpp
    KeyTest.cpp
    MouseButtonTest.cpp
    NullInputBackendTest.cpp
    PositionTest.cpp
)

target_link_libraries(antwika_input_tests
    PRIVATE
        antwika::input
        antwika::log::tests::mocks
        gtest::gtest
)

include(GoogleTest)
gtest_discover_tests(antwika_input_tests)
