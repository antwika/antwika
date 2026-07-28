#pragma once

/**
 * @file
 * @brief Names of events defined by this application.
 */
namespace antwika::game::events
{

    /**
     * @brief An application-defined event, unknown to the engine core.
     *
     * Uses the same TimedEvent/ITimedEventSink pipeline as built-in events
     * (see antwika::engine::events::kTick for a built-in example). The
     * payload is the amount to add to GameState::score, encoded as a plain
     * base-10 integer string — an app-chosen encoding the engine has no
     * opinion about.
     */
    inline constexpr const char *kScoreIncrement = "game.score_increment";

} // namespace antwika::game::events
