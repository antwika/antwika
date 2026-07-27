#pragma once

namespace antwika::game::events
{

    // An application-defined event, unknown to the engine core, flowing
    // through the exact same TimedEvent/ITimedEventSink pipeline as the
    // engine's own built-in events (see antwika::engine::events::kTick).
    // The payload is the amount to add to GameState::score, encoded as a
    // plain base-10 integer string -- an app-chosen encoding the engine
    // has no opinion about.
    inline constexpr const char *kScoreIncrement = "game.score_increment";

} // namespace antwika::game::events
