#pragma once

namespace antwika::life
{

    /**
     * @brief Whether the board is being drawn on right now.
     *
     * The one fact two collaborators have to agree on: PointerToggleSink
     * sets it from the presses and releases it decodes, and
     * DragPausedSystem reads it to decide whether a generation runs. A
     * small shared state object rather than one asking the other, so the
     * system that pauses does not have to know what a pointer is, and the
     * sink does not have to know that anything is watching.
     *
     * It holds no input of its own and is folded purely from recorded
     * events, so a replay rebuilds the same sequence of pauses and reaches
     * the same board. Nothing here reads a device or a clock.
     */
    class DragState final
    {
    public:
        /**
         * @brief Note that a drag has started.
         *
         * Idempotent: a window system is free to report a press without a
         * release before it, and a drag that is already under way simply
         * stays under way.
         */
        void begin() noexcept;

        /**
         * @brief Note that the drag has finished.
         */
        void end() noexcept;

        /**
         * @brief Check whether a drag is under way.
         * @return True between a begin() and the end() that follows it.
         */
        [[nodiscard]] bool inProgress() const noexcept;

    private:
        bool dragging = false;
    };

} // namespace antwika::life
