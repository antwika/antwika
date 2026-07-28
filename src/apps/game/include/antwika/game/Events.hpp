#pragma once

namespace antwika::game::events
{

    // An application-defined event, unknown to the engine core.
    // Uses the same TimedEvent/ITimedEventSink pipeline as built-in events.
    // See antwika::engine::events::kTick for a built-in example.
    // The payload is the amount to add to GameState::score.
    // It's encoded as a plain base-10 integer string.
    // That's an app-chosen encoding the engine has no opinion about.
    inline constexpr const char *kScoreIncrement = "game.score_increment";

} // namespace antwika::game::events
