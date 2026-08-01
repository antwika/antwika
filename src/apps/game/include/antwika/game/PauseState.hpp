#pragma once

namespace antwika::game
{

    /**
     * @brief Whether the simulation is being held still right now.
     *
     * **This is simulation state**, in exactly the sense the camera and
     * the selected tool are: what a run computes depends on it, so it is
     * regenerated from the recorded clicks rather than recorded itself.
     * No event of any kind is defined for pausing -- UiSink resolves the
     * press against the toolbar inside the tick path and toggles this,
     * so a replay pauses on precisely the ticks the live run did.
     *
     * The one fact two collaborators have to agree on: UiSink writes it
     * from the button it resolved, and PauseGatedSystem reads it to
     * decide whether a system runs. A small shared state object rather
     * than one asking the other, so the systems that stop need not know
     * what a button is -- the same shape as life::DragState, which is
     * where this pattern comes from.
     *
     * Nothing here reads a device or a clock.
     */
    class PauseState final
    {
    public:
        /**
         * @brief Turn the pause on if it is off, and off if it is on.
         */
        void toggle() noexcept;

        /**
         * @brief Hold the simulation still, whether or not it already
         * is.
         *
         * What a city's screen coming up asks for -- see CityEntrySink.
         * A toggle would be wrong there: it would let a city entered
         * from a paused one come up running, so what a fresh city did
         * would depend on what the last one was doing.
         */
        void hold() noexcept;

        /**
         * @brief Let the simulation run again, whether or not it was
         * held.
         *
         * hold()'s counterpart, and what the end of a road drag asks for
         * -- see RoadDrag. A toggle would be wrong there for hold()'s
         * reason read backwards: a drag holds the run so that what it is
         * planned against cannot move under it, and a toggle would then
         * pause a run that was already running.
         *
         * Who may call it is the caller's rule rather than this class's:
         * RoadDrag::heldForDrag() is what keeps a drag from resuming a
         * run somebody paused for themselves.
         */
        void release() noexcept;

        /**
         * @brief Check whether the simulation is held still.
         * @return True while paused, which a run begins not being.
         */
        [[nodiscard]] bool paused() const noexcept;

    private:
        bool held = false;
    };

} // namespace antwika::game
