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
         * @brief Check whether the simulation is held still.
         * @return True while paused, which a run begins not being.
         */
        [[nodiscard]] bool paused() const noexcept;

    private:
        bool held = false;
    };

} // namespace antwika::game
