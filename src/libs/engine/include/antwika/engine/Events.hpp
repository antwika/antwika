#pragma once

/**
 * @file
 * @brief Names of events dispatched by the engine itself.
 */
namespace antwika::engine::events
{

    /**
     * @brief Dispatched by Engine::step() at the start of every fixed tick,
     * before that tick's queued events are processed.
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
