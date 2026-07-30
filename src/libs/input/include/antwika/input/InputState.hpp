#pragma once

#include "antwika/input/InputEvent.hpp"
#include "antwika/input/Keyboard.hpp"
#include "antwika/input/Mouse.hpp"

namespace antwika::input
{

    /**
     * @brief Both devices' folded state, and the one thing that folds an
     * InputEvent.
     *
     * Holding this is what an application does with the edges an
     * IInputBackend reports, or with the ones a replay carries: apply()
     * every event, then ask Keyboard and Mouse the questions.
     *
     * Folding happens here rather than inside the backend because a
     * replay must carry the edges and regenerate the state -- the same
     * rule engine.tick follows. State reported across the seam could not
     * be replayed without persisting it.
     */
    class InputState final
    {
    public:
        /**
         * @brief Forget both devices' per-tick edges.
         *
         * Call this once per tick, before applying that tick's events.
         * Reading an edge does not clear it, so that two systems asking
         * the same question in one tick get the same answer -- see
         * Keyboard.
         */
        void beginTick() noexcept;

        /**
         * @brief Fold one event into whichever device it belongs to.
         *
         * A pointer event's modifiers reach the keyboard, because that is
         * where a held modifier lives -- a shift key is a key whichever
         * event happened to mention it, and an action bound to
         * shift-and-a-mouse-button has to be able to ask.
         *
         * @param event The event to apply.
         */
        void apply(const InputEvent &event) noexcept;

        /**
         * @brief Get the keyboard's folded state.
         * @return The keyboard.
         */
        [[nodiscard]] const Keyboard &keyboard() const noexcept;

        /**
         * @brief Get the pointer's folded state.
         * @return The mouse.
         */
        [[nodiscard]] const Mouse &mouse() const noexcept;

    private:
        Keyboard keys;
        Mouse pointer;
    };

} // namespace antwika::input
