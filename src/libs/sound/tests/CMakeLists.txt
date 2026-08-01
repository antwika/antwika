find_package(GTest REQUIRED)

add_subdirectory(conformance)
add_subdirectory(mocks)

add_executable(antwika_sound_tests
    MixerTest.cpp
    NullDeviceTest.cpp
    NullSoundBackendConformanceTest.cpp
    NullSoundBackendTest.cpp
    OfflineDeviceTest.cpp
    SampleBufferTest.cpp
    SoundCapabilitiesTest.cpp
    SoundErrorTest.cpp
    WavReaderTest.cpp
    WaveFormatTest.cpp
    WaveformIdTest.cpp
    WaveformLibraryTest.cpp
    WaveformTest.cpp
)

target_link_libraries(antwika_sound_tests
    PRIVATE
        antwika::sound
        antwika::sound::tests::conformance
        antwika::sound::tests::mocks
        gtest::gtest
)

antwika_bundle_test(TARGET antwika_sound_tests)
