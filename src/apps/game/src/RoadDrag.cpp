#include "antwika/game/RoadDrag.hpp"

namespace antwika::game
{

    void RoadDrag::begin(Cell cell) noexcept
    {
        dragging = true;
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

}
