# Running a simulation, which a replay is only one way to feed.
# EngineLoop is the one code path a live run and a replayed one share,
# and a live run is not a replay at all -- so the loop and the pacing
# around it live here, and the file format that can supply the loop's
# seam lives in antwika::replay, which depends on this library and never
# the other way round.
#
# The ITickEventSource seam itself lives in antwika::event, not here:
# seven of its implementers are antwika::input decorators and two are
# this library's, and a library implementing an interface should not
# have to link the one place that happens to call it.
#
# antwika::ecs comes in for TickPacer, which is an ecs::ISystem.
# antwika::gfx is deliberately absent: a tick loop has no business
# knowing what a window is, which is why WindowInputSource -- the one
# thing here that did -- moved to antwika::app.
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
