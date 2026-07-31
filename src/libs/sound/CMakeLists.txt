antwika_add_library(
    NAME sound
    SOURCES
        src/Mixer.cpp
        src/NullDevice.cpp
        src/NullSoundBackend.cpp
        src/OfflineDevice.cpp
        src/SampleBuffer.cpp
        src/WavReader.cpp
        src/WaveFormat.cpp
        src/Waveform.cpp
        src/WaveformLibrary.cpp
    DEPENDS
        antwika::log
)
