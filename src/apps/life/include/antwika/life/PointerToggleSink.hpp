#pragma once

#include <map>
#include <optional>
#include <set>

#include <antwika/ecs/Entity.hpp>
#include <antwika/ecs/World.hpp>
#include <antwika/event/ITickEventSink.hpp>
#include <antwika/gfx/Size.hpp>
#include <antwika/input/IInputEventCodec.hpp>
#include <antwika/input/Position.hpp>

#include <antwika/geometry/Grid.hpp>
#include "antwika/life/DragState.hpp"
#include "antwika/life/Grid.hpp"

namespace antwika::life
{

    using antwika::ecs::Entity;
    using antwika::ecs::World;
    using antwika::event::ITickEventSink;
    using antwika::event::TickEvent;
    using antwika::gfx::Size;
    using antwika::input::IInputEventCodec;
    using antwika::input::Position;

    class PointerToggleSink final : public ITickEventSink
    {
    public:
        PointerToggleSink(
            World &world,
            const Grid &grid,
            const IInputEventCodec &codec,
            Size canvas,
            DragState &drag);

        PointerToggleSink(const PointerToggleSink &) = delete;
        PointerToggleSink(PointerToggleSink &&) = delete;

        PointerToggleSink &operator=(const PointerToggleSink &) = delete;
        PointerToggleSink &operator=(PointerToggleSink &&) = delete;

        void handle(const TickEvent &event) override;

        [[nodiscard]] const std::set<Entity> &
        visitedCells() const noexcept;

        [[nodiscard]] const std::optional<Position> &
        lastDragPosition() const noexcept;

        void restoreDrag(
            std::set<Entity> visitedCells,
            std::optional<Position> lastDragPosition);

    private:
        void toggleAt(Position position);

        void toggleAlong(Position from, Position to);

        World &world;
        const Grid &grid;
        const IInputEventCodec &codec;
        DragState &drag;
        Size canvas;
        std::optional<antwika::geometry::Grid> layout;
        std::set<Entity> visited;
        std::map<Entity, bool> staged;

        std::optional<Position> lastDrag;
    };

}
