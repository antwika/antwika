antwika_add_library(
    NAME engine
    SOURCES
        src/Engine.cpp
        src/EngineLoop.cpp
        src/StopSignal.cpp
    DEPENDS
        antwika::log
        antwika::event
        antwika::time
)
