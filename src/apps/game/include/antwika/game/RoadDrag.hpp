#pragma once

#include "antwika/game/Cell.hpp"

namespace antwika::game
{

    /**
     * @brief Where a road drag started and where it is being taken.
     *
     * **This is simulation state**, in exactly the sense the camera, the
     * selected tool and PauseState are: which cell a press means depends
     * on it, and what a release lays is a function of it, so it is
     * regenerated from the recorded presses, movements and releases
     * rather than recorded itself. No `game.*` event exists for a drag,
     * for the reason none exists for laying a tile -- see Events.hpp.
     *
     * It may therefore never be written from input::PointerHintChannel,
     * which a replay does not reproduce. GridSink writes it inside the
     * tick path from the recorded stream and nothing else does; a
     * renderer only reads it, exactly as it only reads PauseState.
     *
     * The one fact three collaborators have to agree on: GridSink writes
     * it, planRoad() is asked about it, and the renderer draws the answer.
     * A small shared state object rather than one asking the other, which
     * is the shape PauseState and life::DragState already have.
     */
    class RoadDrag final
    {
    public:
        /**
         * @brief Start a drag on a cell.
         *
         * @param cell The cell the press landed on, which is both the
         * start and -- until the pointer moves -- the end.
         * @param alreadyHeld Whether the run was already paused when the
         * press arrived. Remembered so that ending the drag can leave a
         * run somebody paused for themselves paused -- see
         * heldForDrag().
         */
        void begin(Cell cell, bool alreadyHeld) noexcept;

        /**
         * @brief Take the drag to another cell.
         *
         * Does nothing while no drag is under way, so a movement with no
         * button behind it cannot leave a stale end behind.
         *
         * @param cell The cell the pointer is over now.
         */
        void dragTo(Cell cell) noexcept;

        /** @brief End the drag, whether or not one was under way. */
        void finish() noexcept;

        /**
         * @brief Check whether a drag is under way.
         * @return True between a press and its release.
         */
        [[nodiscard]] bool active() const noexcept;

        /**
         * @brief Get the cell the drag started on.
         * @return The cell, which is the origin only while active().
         */
        [[nodiscard]] Cell start() const noexcept;

        /**
         * @brief Get the cell the drag has reached.
         * @return The last cell dragTo() was given, or the start.
         */
        [[nodiscard]] Cell end() const noexcept;

        /**
         * @brief Check whether the pause is this drag's to release.
         *
         * **A drag must not fight the player's own pause.** A drag holds
         * the run still so that what it is planned against cannot move
         * under it, and releasing it afterwards would resume a city
         * somebody had deliberately paused before they touched the
         * mouse. So the pause is released at the end of a drag only when
         * the drag was what held it.
         *
         * @return True when the run was running when the drag began.
         */
        [[nodiscard]] bool heldForDrag() const noexcept;

    private:
        bool dragging = false;
        bool ours = false;
        Cell from{};
        Cell to{};
    };

} // namespace antwika::game
