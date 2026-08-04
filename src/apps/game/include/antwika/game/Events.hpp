#pragma once
#include <antwika/event/ITickEventSource.hpp>

/**
 * @file
 * @brief Names of events defined by this application.
 */
namespace antwika::game::events
{

    using antwika::event::ITickEventSource;

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
     * @brief One action, and the key the machine this run started on
     * has bound to it.
     *
     * **The one thing in this application that is externally supplied
     * and cannot be worked out again**, which is exactly why it is an
     * event and everything else here is not. A click can be resolved
     * against the layout it landed on; a key binding read off the
     * player's own options file is a fact about the machine, and no
     * amount of replaying the clicks recovers it.
     *
     * So it enters a run the way every other externally-supplied input
     * does -- through an ITickEventSource, upstream of the recorder --
     * and a --record file carries it. A replay then takes the layout
     * from the recording rather than from the machine it is being
     * replayed on, which is the whole property BindingReplayTest
     * asserts: the same session, on a machine bound differently, is the
     * same city.
     *
     * The payload is a JSON object of two strings, "action" and "key",
     * both persisted names -- see BindingEvent.hpp.
     *
     * **A rebinding made on the options screen is not this event.**
     * That one is a key press resolved against a layout inside the tick
     * path, downstream of the recorder, and a replay works it out again
     * exactly as it works out which tile a click laid. Recording both
     * would apply it twice.
     */
    inline constexpr const char *kBindKey = "game.bind_key";

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
