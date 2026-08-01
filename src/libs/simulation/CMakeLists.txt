# Running a simulation, which a replay is only one way to feed.
# EngineLoop is the one code path a live run and a replayed one share,
# and a live run is not a replay at all -- so the loop, the ITickEventSource
# seam it reads through and the pacing around it live here, and the file
# format that can supply that seam lives in antwika::replay, which
# depends on this library and never the other way round.
#
# antwika::ecs comes in for TickPacer, which is an ecs::ISystem,
# antwika::gfx for WindowInputSource, which turns a window's close
# request into the engine.stop event that ends a run.
# Both are one-way: neither library knows this one exists.
antwika_add_library(
    NAME simulation
    SOURCES
        src/EngineLoop.cpp
        src/TickPacer.cpp
        src/WindowInputSource.cpp
    DEPENDS
        antwika::ecs
        antwika::engine
        antwika::event
        antwika::gfx
        antwika::time
)
