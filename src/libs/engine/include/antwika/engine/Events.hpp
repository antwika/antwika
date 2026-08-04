#pragma once
#include <antwika/event/ITickEventSource.hpp>

/**
 * @file
 * @brief Names of events dispatched by the engine itself.
 */
namespace antwika::engine::events
{

    using antwika::event::ITickEventSource;

    /**
     * @brief Dispatched by Engine::step() once per fixed tick, after
     * that tick's input events have been dispatched.
     *
     * simulation::EngineLoop is what orders the two: it asks its
     * ITickEventSource for the tick's events and dispatches them, and
     * only then steps the engine. So a sink seeing this has already
     * seen everything the source supplied for the same tick, which is
     * what makes it the place to do a tick's end-of-frame work.
     *
     * Application code can react to simulation progress this way,
     * without needing to invent and dispatch its own event for it.
     */
    inline constexpr const char *kTick = "engine.tick";

    /**
     * @brief Dispatched by application code (or a test) to request that the
     * run stop.
     *
     * Observed by StopSignal, which EngineLoop consults after each tick to
     * decide whether to keep running. Unlike kTick, this event is genuine
     * external input, not something the engine regenerates itself -- it
     * must be part of a run's recorded/replayed input for a replay to stop
     * at the same tick a live run did.
     */
    inline constexpr const char *kStop = "engine.stop";

} // namespace antwika::engine::events
