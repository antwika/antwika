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
         * **A drag holds nothing still.** It used to pause the run so
         * that what it was planned against could not move under it, and
         * a city now runs all the time unless a player has asked for a
         * pause -- so a route is planned against a moving city, and the
         * bookkeeping about whose pause this was is gone with it.
         *
         * @param cell The cell the press landed on, which is both the
         * start and -- until the pointer moves -- the end.
         */
        void begin(Cell cell) noexcept;

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

    private:
        bool dragging = false;
        Cell from{};
        Cell to{};
    };

} // namespace antwika::game
