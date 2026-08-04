antwika_add_library(
    NAME console
    SOURCES
        src/ConsolePicture.cpp
        src/ConsoleGatedSink.cpp
        src/ConsoleMount.cpp
        src/ConsoleScene.cpp
        src/ConsoleSink.cpp
        src/ConsoleState.cpp
        src/InputFold.cpp
        src/KeyboardLayout.cpp
        src/SnapshotCommands.cpp
        src/SnapshotFormat.cpp
        src/Typing.cpp
    DEPENDS
        antwika::animation
        # For PointerReading.hpp, which is now one definition there and
        # a re-export here.
        # Every application linking antwika::console already links
        # antwika::app, so no application gains a library here.
        antwika::app
        antwika::config
        antwika::engine
        antwika::event
        antwika::gfx
        antwika::input
        antwika::replay
        antwika::time
        antwika::tween
        antwika::ui
)
