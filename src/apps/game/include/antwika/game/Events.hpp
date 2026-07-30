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
     * @brief There is deliberately no event here for placing a path, a
     * building or a walker, nor for choosing what to build.
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
     * Which tool is selected is the same story one step earlier.
     * It changes on a number key, which arrives as input.key_down and is
     * recorded; a game.select_tool of our own would be the key press
     * written down twice.
     *
     * Nor is there one for a walker being spawned, a delivery, or a
     * building burning down: all three are worked out from the state a
     * replay has already rebuilt, on the tick they happen, by
     * BuildingSystem and WalkerSystem.
     * Recording them would be recording what the engine can regenerate.
     *
     * For the same reason, no input.* name may ever be added to an app's
     * self-generated deny-list -- that list is what saveReplayFile filters,
     * and an input.* name in it would stop recording the only input a live
     * run has.
     */

} // namespace antwika::game::events
