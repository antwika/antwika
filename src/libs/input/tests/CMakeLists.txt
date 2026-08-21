find_package(GTest REQUIRED)

add_subdirectory(conformance)
add_subdirectory(fakes)
add_subdirectory(mocks)

add_executable(antwika_input_tests
    ActionMapTest.cpp
    BufferedInputSourceTest.cpp
    CoalescingPointerSourceTest.cpp
    DirectionKeysTest.cpp
    IdleMotionFilterTest.cpp
    InMemoryClipboardTest.cpp
    InputCapabilitiesTest.cpp
    InputDeterminismTest.cpp
    InputEventCodecTest.cpp
    InputEventTest.cpp
    InputPipelineTest.cpp
    InputStateTest.cpp
    KeyModifiersTest.cpp
    KeyTest.cpp
    KeyTextTest.cpp
    KeyboardTest.cpp
    LiveInputSourceTest.cpp
    MappedPointerSourceTest.cpp
    MouseButtonTest.cpp
    MouseTest.cpp
    NullInputBackendConformanceTest.cpp
    NullInputBackendTest.cpp
    OffsetTest.cpp
    PointerHintChannelTest.cpp
    PointerHintSourceTest.cpp
    PointerHintTest.cpp
    PositionTest.cpp
    StopOnKeySourceTest.cpp
)

target_link_libraries(antwika_input_tests
    PRIVATE
        antwika::input
        antwika::input::tests::conformance
        antwika::input::tests::fakes
        antwika::input::tests::mocks
        antwika::log::tests::mocks
        antwika::replay
        antwika::testing
        gtest::gtest
)

antwika_bundle_test(TARGET antwika_input_tests)
