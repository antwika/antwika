antwika_add_library(
    NAME simulation
    SOURCES
        src/EngineLoop.cpp
        src/TickPacer.cpp
    DEPENDS
        antwika::ecs
        antwika::engine
        antwika::event
        antwika::time
)
