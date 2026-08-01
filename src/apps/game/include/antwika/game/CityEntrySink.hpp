#pragma once

#include <antwika/event/ITickEventSink.hpp>
#include <antwika/event/TickEvent.hpp>

#include "antwika/game/AppMode.hpp"
#include "antwika/game/PauseState.hpp"

namespace antwika::game
{

    using antwika::event::ITickEventSink;
    using antwika::event::TickEvent;

    /**
     * @brief Holds the simulation still whenever a city's screen comes
     * up.
     *
     * **A city is entered paused**, so progress is something a player
     * asks for rather than something that starts happening at them the
     * moment a grid appears. The one way out is the toolbar's pause
     * button, which UiSink already resolves.
     *
     * It watches the *transition* into AppMode::CityMap rather than the
     * mode itself, so a run that is already on a city's grid is left
     * alone: holding the pause every tick would make the button do
     * nothing at all.
     *
     * That is also what decides the question a session's first city
     * asks. Every route in is a transition -- New Game from the menu, a
     * city picked off the world map, a save loaded -- so all three come
     * up paused and the rule needs no exception. A test that constructs
     * its AppModeState already in AppMode::CityMap never transitions and
     * so is never held, which is the same latitude AppModeState's
     * initial mode already gives one.
     *
     * **Nothing here is persisted and no event is defined for it**, in
     * exactly PauseState's sense: this runs inside the tick path, off
     * the mode a replay regenerates from the recorded clicks, so a
     * replay is held on precisely the ticks the live run was. Nothing
     * here reads a device or a clock.
     *
     * Register it immediately after AppModeState, which commits the
     * mode, and ahead of GridSink, which runs the systems the pause
     * stops.
     */
    class CityEntrySink final : public ITickEventSink
    {
    public:
        /**
         * @brief Construct the sink over the mode it watches.
         * @param mode The app's mode; the mode it is in at construction
         * is the one this starts from, so a run constructed on a city's
         * grid has already entered it. Must outlive this sink.
         * @param pause The pause a city coming up holds. Must outlive
         * this sink.
         */
        CityEntrySink(
            const AppModeState &mode, PauseState &pause) noexcept;

        CityEntrySink(const CityEntrySink &) = delete;
        CityEntrySink(CityEntrySink &&) = delete;

        CityEntrySink &operator=(const CityEntrySink &) = delete;
        CityEntrySink &operator=(CityEntrySink &&) = delete;

        /**
         * @brief Apply a tick event.
         * @param event engine.tick is the one that acts, since that is
         * the boundary AppModeState commits a change on; anything else
         * is ignored, since no input changes the mode part-way through a
         * tick.
         */
        void handle(const TickEvent &event) override;

    private:
        const AppModeState &mode;
        PauseState &pause;
        AppMode last;
    };

} // namespace antwika::game
