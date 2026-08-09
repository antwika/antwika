find_package(GTest REQUIRED)

add_executable(antwika_synth_tests
    AdsrTest.cpp
    FilterTest.cpp
    OscillateTest.cpp
    SynthErrorTest.cpp
    SynthMixerTest.cpp
    TriggerRequestTest.cpp
    VoiceDescTest.cpp
    WaveshapeTest.cpp
)

target_link_libraries(antwika_synth_tests
    PRIVATE
        antwika::synth
        gtest::gtest
)

antwika_bundle_test(TARGET antwika_synth_tests)
