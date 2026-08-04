#include "antwika/life/PointerToggleSink.hpp"

#include <cstdint>
#include <cstdlib>
#include <utility>
#include <variant>

#include <antwika/engine/Events.hpp>
#include <antwika/input/InputEvent.hpp>
#include <antwika/input/MouseButton.hpp>

#include "antwika/life/Cell.hpp"

namespace antwika::life
{

    using antwika::input::MouseButton;
    using antwika::input::PointerButtonPressed;
    using antwika::input::PointerButtonReleased;
    using antwika::input::PointerMoved;

    // The layout is worked out once, here, rather than per pointer event.
    // Neither the canvas nor the grid's size can change afterwards.
    PointerToggleSink::PointerToggleSink(
        World &world,
        const Grid &grid,
        const IInputEventCodec &codec,
        Size canvas,
        DragState &drag)
        : world(world),
          grid(grid),
          codec(codec),
          drag(drag),
          canvas(canvas),
          layout(layoutFor(canvas, grid.width(), grid.height()))
    {
    }

    void PointerToggleSink::handle(const TickEvent &event)
    {
        // A tick is when BoardSink commits what this staged.
        // A generation runs over it, so the note of it is stale.
        if (event.event.name == antwika::engine::events::kTick)
        {
            staged.clear();
            return;
        }

        const auto edge = codec.decode(event.event);

        if (!edge)
        {
            return;
        }

        if (const auto *pressed =
                std::get_if<PointerButtonPressed>(&*edge))
        {
            if (pressed->button == MouseButton::Left)
            {
                drag.begin();

                // Cleared on the press, not on the release.
                // That also copes with a press nothing preceded.
                visited.clear();
                lastDrag = pressed->position;
                toggleAt(pressed->position);
            }

            return;
        }

        if (const auto *released =
                std::get_if<PointerButtonReleased>(&*edge))
        {
            if (released->button == MouseButton::Left)
            {
                drag.end();
                lastDrag.reset();
            }

            return;
        }

        if (const auto *moved = std::get_if<PointerMoved>(&*edge))
        {
            if (drag.inProgress())
            {
                // The window system samples a drag discretely.
                // A fast one jumps several cells between two events.
                // So the whole segment is walked, never just its end.
                toggleAlong(
                    lastDrag.value_or(moved->position),
                    moved->position);

                lastDrag = moved->position;
            }
        }
    }

    const std::set<Entity> &
    PointerToggleSink::visitedCells() const noexcept
    {
        return visited;
    }

    const std::optional<Position> &
    PointerToggleSink::lastDragPosition() const noexcept
    {
        return lastDrag;
    }

    void PointerToggleSink::restoreDrag(
        std::set<Entity> visitedCells,
        std::optional<Position> lastDragPosition)
    {
        visited = std::move(visitedCells);
        lastDrag = lastDragPosition;
        staged.clear();
    }

    // The integer Bresenham atlas_editor already strokes with.
    // The visited set keeps once-per-drag intact along the way.
    // Integer throughout: which cell a drag means is simulation state.
    void PointerToggleSink::toggleAlong(
        const Position from, const Position to)
    {
        // A recording is hand-editable, and this walk is per pixel.
        // A crafted jump wider than the canvas toggles its end alone.
        // The board is inside the canvas, so no cell is missable.
        const std::int64_t bound =
            std::int64_t{canvas.width} + std::int64_t{canvas.height};

        const auto tall = std::abs(
            std::int64_t{to.y} - std::int64_t{from.y});
        const auto wide = std::abs(
            std::int64_t{to.x} - std::int64_t{from.x});

        if (wide + tall > bound)
        {
            toggleAt(to);

            return;
        }

        const std::int32_t stepX = to.x < from.x ? -1 : 1;
        const std::int32_t stepY = to.y < from.y ? -1 : 1;

        // Distances as magnitudes, the vertical one negated.
        // That is the form the two comparisons below are written for.
        const std::int32_t spanX = (to.x - from.x) * stepX;
        const std::int32_t spanY = (from.y - to.y) * stepY;

        std::int32_t error = spanX + spanY;
        Position walked = from;

        for (;;)
        {
            toggleAt(walked);

            if (walked.x == to.x && walked.y == to.y)
            {
                return;
            }

            const std::int32_t doubled = 2 * error;

            if (doubled >= spanY)
            {
                error += spanY;
                walked.x += stepX;
            }

            if (doubled <= spanX)
            {
                error += spanX;
                walked.y += stepY;
            }
        }
    }

    void PointerToggleSink::toggleAt(Position position)
    {
        if (!layout)
        {
            return;
        }

        const auto cell = cellAt(*layout, position.x, position.y);

        if (!cell)
        {
            return;
        }

        // The entity is the cell's identity, so both notes key on it.
        // How the grid addresses a cell stays the grid's business.
        const auto entity = grid.entityAt(cell->x, cell->y);

        // The walked segment lands on one cell many pixels over.
        // And a drag may cross a cell it toggled earlier.
        // Toggling on each would tie the result to how it was drawn.
        if (!visited.insert(entity).second)
        {
            return;
        }

        // World hands out the committed value.
        // So a cell already staged this tick still reads as it was.
        // Two drags over one cell in a tick would collapse into one.
        const auto alreadyStaged = staged.find(entity);
        const bool wasAlive = alreadyStaged != staged.end()
                                  ? alreadyStaged->second
                                  : world.get<Cell>(entity).alive;

        staged[entity] = !wasAlive;
        world.set<Cell>(entity, Cell{.alive = !wasAlive});
    }

} // namespace antwika::life
