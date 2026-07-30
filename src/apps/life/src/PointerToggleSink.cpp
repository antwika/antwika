#include "antwika/life/PointerToggleSink.hpp"

#include <variant>

#include <antwika/engine/Events.hpp>
#include <antwika/input/InputEvent.hpp>
#include <antwika/input/MouseButton.hpp>

#include "antwika/life/BoardLayout.hpp"
#include "antwika/life/Cell.hpp"

namespace antwika::life
{

    using antwika::input::MouseButton;
    using antwika::input::PointerButtonPressed;
    using antwika::input::PointerButtonReleased;
    using antwika::input::PointerMoved;

    PointerToggleSink::PointerToggleSink(
        World &world,
        const Grid &grid,
        const IInputEventCodec &codec,
        Size canvas)
        : world(world), grid(grid), codec(codec), canvas(canvas)
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
                visited.clear();
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
        const auto layout = layoutFor(canvas, grid.width(), grid.height());

        if (!layout)
        {
            return;
        }

        const auto cell = cellAt(*layout, position.x, position.y);

        if (!cell)
        {
            return;
        }

        // A drag reports a position per pixel.
        // So the same cell arrives many times over.
        // Toggling on each would tie the result to how fast it was drawn.
        const auto index =
            static_cast<std::uint64_t>(cell->y) * grid.width() + cell->x;

        if (!visited.insert(index).second)
        {
            return;
        }

        const auto entity = grid.entityAt(cell->x, cell->y);

        // World hands out the committed value.
        // So a cell already staged this tick still reads as it was.
        // Two drags over one cell in a tick would collapse into one.
        const auto alreadyStaged = staged.find(index);
        const bool wasAlive = alreadyStaged != staged.end()
                                  ? alreadyStaged->second
                                  : world.get<Cell>(entity).alive;

        staged[index] = !wasAlive;
        world.set<Cell>(entity, Cell{.alive = !wasAlive});
    }

} // namespace antwika::life
