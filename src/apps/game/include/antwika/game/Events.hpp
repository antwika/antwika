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
     * payload is a JSON object with an unsigned integer "amount" field to
     * add to GameState::score — an app-chosen encoding the engine has no
     * opinion about.
     */
    inline constexpr const char *kScoreIncrement = "game.score_increment";

} // namespace antwika::game::events
