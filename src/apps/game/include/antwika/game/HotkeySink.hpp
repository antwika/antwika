#pragma once

#include <antwika/event/ITickEventSink.hpp>
#include <antwika/event/TickEvent.hpp>

#include "antwika/game/InputFold.hpp"
#include "antwika/game/OptionsState.hpp"
#include "antwika/game/ViewCommands.hpp"

namespace antwika::game
{

    using antwika::event::ITickEventSink;
    using antwika::event::TickEvent;

    /**
     * @brief Turns a bound key press into the thing it was bound to.
     *
     * **The bindings define no event of their own here.** A key press is
     * the input; which action it asks for is resolved against the run's
     * layout inside the tick path and downstream of the recorder, and
     * the pause or the camera move is regenerated from it on replay --
     * exactly as GridSink regenerates a placement from a click. The one
     * thing that *is* an event is the layout the machine started with,
     * which nothing in the run could work out again; see Events.hpp.
     *
     * A repeat is not a fresh press, so holding the pause key holds the
     * run once rather than flickering it.
     *
     * **It is a second writer of PauseState, and that is safe on the
     * terms PauseState sets.** A key press is a player asking for a
     * pause exactly as the bar's button is, and the two never disagree
     * about a tick: whichever runs later says what the run does, and
     * both are folded from one recorded stream, so a replay reaches the
     * same answer by construction. The value it writes is a toggle
     * rather than the button's absolute one because a key press carries
     * no state it was showing -- two presses in one tick are a player
     * pressing twice, which is what toggling twice means.
     *
     * Registered wrapped in a ModeGatedSink for AppMode::CityMap: a
     * hotkey acts on the city, and the options screen where they are
     * bound is a screen of the main menu, so nothing here can fire while
     * a key is being chosen.
     */
    class HotkeySink final : public ITickEventSink
    {
    public:
        /**
         * @brief Construct the sink over everything it drives.
         * @param options The run's bindings. Must outlive this sink, and
         * must be written by a BindingSink registered ahead of it.
         * @param input The folded input, holding the event being
         * handled. Must outlive this sink, and must be registered ahead
         * of it.
         * @param view What a bound action asks for -- the same four
         * verbs the bar's buttons ask for. Must outlive this sink.

         */
        HotkeySink(
            const OptionsState &options,
            const InputFold &input,
            ViewCommands &view) noexcept;

        HotkeySink(const HotkeySink &) = delete;
        HotkeySink(HotkeySink &&) = delete;

        HotkeySink &operator=(const HotkeySink &) = delete;
        HotkeySink &operator=(HotkeySink &&) = delete;

        /**
         * @brief Apply a tick event.
         * @param event An input.* event carrying a fresh press of a
         * bound key is acted on; anything else is ignored.
         */
        void handle(const TickEvent &event) override;

    private:
        void act(Action action) noexcept;

        const OptionsState &options;
        const InputFold &input;
        ViewCommands &view;
    };

} // namespace antwika::game
