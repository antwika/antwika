find_package(GTest REQUIRED)

add_executable(antwika_sequencer_tests
    FrameClockTest.cpp
    SequencerErrorTest.cpp
    SequencerTest.cpp
    TempoMapTest.cpp
)

target_link_libraries(antwika_sequencer_tests
    PRIVATE
        antwika::sequencer
        gtest::gtest
)

antwika_bundle_test(TARGET antwika_sequencer_tests)
