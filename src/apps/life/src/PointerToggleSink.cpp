#include "antwika/life/PointerToggleSink.hpp"

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
        Size canvas)
        : world(world),
          grid(grid),
          codec(codec),
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
                dragging = true;

                // Cleared on the press, not on the release.
                // That also copes with a press nothing preceded.
                visited.clear();
                toggleAt(pressed->position);
            }

            return;
        }

        if (const auto *released =
                std::get_if<PointerButtonReleased>(&*edge))
        {
            if (released->button == MouseButton::Left)
            {
                dragging = false;
            }

            return;
        }

        if (const auto *moved = std::get_if<PointerMoved>(&*edge))
        {
            if (dragging)
            {
                toggleAt(moved->position);
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

        // A drag reports a position per pixel.
        // So the same cell arrives many times over.
        // Toggling on each would tie the result to how fast it was drawn.
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
