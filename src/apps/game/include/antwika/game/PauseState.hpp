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
     * press against the toolbar inside the tick path and sets this, so a
     * replay pauses on precisely the ticks the live run did.
     *
     * **A pause is asked for, and nothing else causes one.**
     * A city coming up, a menu opening and a road being dragged out each
     * used to hold it too, and none of them does now: a run progresses
     * all the time unless a player has said otherwise.
     * That is what leaves this one writer rather than four, and it is
     * why no bookkeeping is left about whose pause a release lets go of.
     *
     * **The value is absolute rather than a toggle**, which is what
     * keeps it right once more than one player can ask for one.
     * Two players pausing on one tick would toggle twice and leave the
     * run going, each having watched themselves press pause; two asking
     * for true leave it paused.
     * So a button sends the opposite of the state it was showing, and
     * every order two such asks can land in ends somewhere both meant.
     *
     * The one fact two collaborators have to agree on: UiSink writes it
     * from the button it resolved, and PauseGatedSystem reads it to
     * decide whether a system runs.
     * A small shared state object rather than one asking the other, so
     * the systems that stop need not know what a button is -- the same
     * shape as life::DragState, which is where this pattern comes from.
     *
     * Nothing here reads a device or a clock.
     */
    class PauseState final
    {
    public:
        /**
         * @brief Hold the simulation still, or let it run again.
         *
         * Idempotent, deliberately: asking for the state it is already
         * in does nothing, so two players asking for the same thing
         * agree rather than cancelling one another out.
         *
         * @param paused True to hold the run still, false to let it go.
         */
        void set(bool paused) noexcept;

        /**
         * @brief Check whether the simulation is held still.
         * @return True while paused, which a run begins not being.
         */
        [[nodiscard]] bool paused() const noexcept;

    private:
        bool held = false;
    };

} // namespace antwika::game
