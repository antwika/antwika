antwika_add_app(
    NAME sound_demo
    SOURCES
        src/DemoLoop.cpp
        src/DemoOptions.cpp
        src/DemoTrack.cpp
        src/main.cpp
    LIBS
        antwika::app
        antwika::cli
        antwika::log
        antwika::sound
        antwika::sound_backend
        antwika::time
)
