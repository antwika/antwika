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

} // namespace antwika::engine::events
