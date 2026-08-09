antwika_add_library(
    NAME synth
    SOURCES
        src/Adsr.cpp
        src/Filter.cpp
        src/Oscillate.cpp
        src/SynthMixer.cpp
        src/VoiceDesc.cpp
        src/Waveshape.cpp
    DEPENDS
        antwika::enums
        antwika::rng
        antwika::sound
)
