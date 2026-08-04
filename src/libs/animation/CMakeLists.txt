antwika_add_library(
    NAME animation
    SOURCES
        src/Clip.cpp
        src/Playback.cpp
        src/Progress.cpp
    DEPENDS
        antwika::time
)
