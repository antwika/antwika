# The one module that may depend on several others at once.
# Everything here is an application saying how two libraries that know
# nothing of each other fit together, which is exactly the sentence no
# library below may contain.
antwika_add_library(
    NAME app
    SOURCES
        src/WindowInputSource.cpp
        src/AssetPath.cpp
        src/ConsoleLogging.cpp
        src/FramePacedSource.cpp
        src/FullscreenToggleSource.cpp
        src/MaxTicks.cpp
        src/PngFile.cpp
        src/PointerReading.cpp
        src/RunGuarded.cpp
        src/RunRecorded.cpp
        src/TickLimitSource.cpp
        src/WavFile.cpp
        src/WindowCloseSource.cpp
        src/WindowedSession.cpp
        src/WindowPointerMapping.cpp
    DEPENDS
        antwika::animation
        antwika::cli
        antwika::event
        antwika::gfx
        antwika::input
        antwika::io
        antwika::log
        antwika::replay
        antwika::simulation
        antwika::sound
        antwika::time
        antwika::ui
)
