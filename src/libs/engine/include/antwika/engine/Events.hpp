#pragma once

namespace antwika::engine::events
{

    // Dispatched by Engine::step() at the start of every fixed tick.
    // This happens before that tick's queued events are processed.
    // Application code can react to simulation progress this way.
    // It doesn't need to invent and dispatch its own event for it.
    inline constexpr const char *kTick = "engine.tick";

} // namespace antwika::engine::events
