antwika_add_library(
    NAME sequencer
    SOURCES
        src/FrameClock.cpp
        src/Sequencer.cpp
        src/TempoMap.cpp
    DEPENDS
        antwika::pattern
        antwika::sound
        antwika::time
)
