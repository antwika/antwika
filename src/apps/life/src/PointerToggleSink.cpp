#include "antwika/life/PointerToggleSink.hpp"

#include <cstdint>
#include <cstdlib>
#include <utility>
#include <variant>

#include <antwika/engine/Events.hpp>
#include <antwika/input/InputEvent.hpp>
#include <antwika/input/MouseButton.hpp>

#include <antwika/geometry/Grid.hpp>

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
        Size canvas,
        DragState &drag)
        : world(world),
          grid(grid),
          codec(codec),
          drag(drag),
          canvas(canvas),
          layout(antwika::geometry::gridFit(
              antwika::geometry::Rect{
                  .origin = antwika::geometry::Point{}, .size = canvas},
              grid.width(),
              grid.height()))
    {
    }

    void PointerToggleSink::handle(const TickEvent &event)
    {
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

    void PointerToggleSink::toggleAlong(
        const Position from, const Position to)
    {
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

        const auto cell = antwika::geometry::cellAt(
            *layout,
            antwika::geometry::Point{
                .x = position.x, .y = position.y});

        if (!cell)
        {
            return;
        }

        const auto entity = grid.entityAt(cell->column, cell->row);

        if (!visited.insert(entity).second)
        {
            return;
        }

        const auto alreadyStaged = staged.find(entity);
        const bool wasAlive = alreadyStaged != staged.end()
                                  ? alreadyStaged->second
                                  : world.get<Cell>(entity).alive;

        staged[entity] = !wasAlive;
        world.set<Cell>(entity, Cell{.alive = !wasAlive});
    }

}
