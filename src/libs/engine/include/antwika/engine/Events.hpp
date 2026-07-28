#pragma once

namespace antwika::engine::events
{

    // Dispatched by Engine::step() at the start of every fixed tick, before that tick's queued events are processed, so application code can react to simulation progress without inventing and dispatching its own.
    inline constexpr const char *kTick = "engine.tick";

} // namespace antwika::engine::events
