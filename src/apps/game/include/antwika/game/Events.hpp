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
     * Uses the same TickEvent/ITickEventSink pipeline as built-in events
     * (see antwika::engine::events::kTick for a built-in example). The
     * payload is a JSON object with an unsigned integer "amount" field to
     * add to GameState::score — an app-chosen encoding the engine has no
     * opinion about.
     */
    inline constexpr const char *kScoreIncrement = "game.score_increment";

    /**
     * @brief Announces that the game started.
     *
     * The application generates this itself on every run, so a caller
     * persisting a replay filters it back out rather than storing it --
     * see antwika::replay::saveReplayFile.
     */
    inline constexpr const char *kStarted = "Running Antwika Game";

    /**
     * @brief There is deliberately no event here for placing a path or a
     * walker.
     *
     * A click arrives as antwika::input's input.pointer_down and becomes a
     * placement inside the tick path, in GridSink, which is downstream of
     * the recorder.
     * A replay therefore stores the click and regenerates the placement.
     *
     * An event of our own for the placement would be derived state:
     * TickEventRecorder would write it alongside the click that caused it,
     * and replaying the file would lay two tiles for one click.
     *
     * For the same reason, no input.* name may ever be added to an app's
     * self-generated deny-list -- that list is what saveReplayFile filters,
     * and an input.* name in it would stop recording the only input a live
     * run has.
     */

} // namespace antwika::game::events
