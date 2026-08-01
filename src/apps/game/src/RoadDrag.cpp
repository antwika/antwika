#include "antwika/game/RoadDrag.hpp"

namespace antwika::game
{

    void RoadDrag::begin(Cell cell, bool alreadyHeld) noexcept
    {
        dragging = true;
        ours = !alreadyHeld;
        from = cell;
        to = cell;
    }

    void RoadDrag::dragTo(Cell cell) noexcept
    {
        if (!dragging)
        {
            return;
        }

        to = cell;
    }

    void RoadDrag::finish() noexcept
    {
        dragging = false;

        // The pause is nobody's to release once the drag has ended.
        // Left set, a second release would resume a run nothing held.
        ours = false;
    }

    bool RoadDrag::active() const noexcept
    {
        return dragging;
    }

    Cell RoadDrag::start() const noexcept
    {
        return from;
    }

    Cell RoadDrag::end() const noexcept
    {
        return to;
    }

    bool RoadDrag::heldForDrag() const noexcept
    {
        return ours;
    }

} // namespace antwika::game
