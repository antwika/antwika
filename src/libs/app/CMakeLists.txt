# The one module that may depend on several others at once.
# Everything here is an application saying how two libraries that know
# nothing of each other fit together, which is exactly the sentence no
# library below may contain.
antwika_add_library(
    NAME app
    SOURCES
        src/ConsoleLogging.cpp
        src/FramePacedSource.cpp
        src/PngFile.cpp
        src/PointerReading.cpp
        src/RunGuarded.cpp
        src/RunRecorded.cpp
    DEPENDS
        antwika::animation
        antwika::event
        antwika::gfx
        antwika::input
        antwika::log
        antwika::replay
        antwika::time
        antwika::ui
)
