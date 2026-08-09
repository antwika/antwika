#include "antwika/game/GridSink.hpp"

#include <optional>
#include <variant>

#include <antwika/engine/Events.hpp>
#include <antwika/input/MouseButton.hpp>

#include "antwika/game/Building.hpp"
#include "antwika/game/BuildTool.hpp"
#include "antwika/game/Cell.hpp"
#include "antwika/game/Cost.hpp"
#include "antwika/game/Coverage.hpp"
#include "antwika/game/Demolition.hpp"
#include "antwika/game/Footprint.hpp"
#include "antwika/game/HousingLevel.hpp"
#include "antwika/game/IsoProjection.hpp"
#include "antwika/game/Path.hpp"
#include "antwika/game/Placement.hpp"
#include "antwika/game/PointerReading.hpp"
#include "antwika/game/RoadPlan.hpp"
#include "antwika/game/Ruin.hpp"
#include "antwika/game/Service.hpp"
#include "antwika/game/Walker.hpp"

namespace antwika::game
{

    using antwika::input::MouseButton;
    using antwika::input::PointerButtonPressed;
    using antwika::input::PointerButtonReleased;
    using antwika::input::PointerMoved;
    using antwika::input::PointerScrolled;

    GridSink::GridSink(
        World &world,
        PathIndex &paths,
        Camera &camera,
        GridExtent extent,
        SystemScheduler &scheduler,
        const InputFold &input,
        UiOverlay &overlay,
        const WorldMapState &cities,
        BuildingIndex &built,
        RoadDrag &drag,
        GameState &state,
        GameConfig config)
        : world(world),
          paths(paths),
          camera(camera),
          extent(extent),
          scheduler(scheduler),
          input(input),
          overlay(overlay),
          cities(cities),
          built(built),
          drag(drag),
          state(state),
          config(config)
    {
    }

    void GridSink::handle(const TickEvent &event)
    {
        if (event.event.name == antwika::engine::events::kTick)
        {
            world.commit();
            scheduler.run(world, event.tick);
            return;
        }

        if (!cities.cityOpen())
        {
            return;
        }

        const auto &decoded = input.current();
        if (!decoded.has_value())
        {
            return;
        }

        act(*decoded);
    }

    void GridSink::act(const antwika::input::InputEvent &event)
    {
        const bool claimable =
            std::holds_alternative<PointerButtonPressed>(event)
            || std::holds_alternative<PointerScrolled>(event);

        if (overlay.pointerOverUi() && claimable)
        {
            return;
        }

        if (const auto *moved = std::get_if<PointerMoved>(&event))
        {
            drag.dragTo(screenToCell(asPoint(moved->position), camera));

            if (input.state().mouse().isDown(MouseButton::Middle))
            {
                const auto previous = input.pointerBefore();

                camera.panBy(
                    moved->position.x - previous.x,
                    moved->position.y - previous.y);
            }

            return;
        }

        if (const auto *pressed = std::get_if<PointerButtonPressed>(&event))
        {
            const auto cell =
                screenToCell(asPoint(pressed->position), camera);

            if (pressed->button == MouseButton::Left)
            {
                begin(cell, overlay.tool());
            }
            else if (pressed->button == MouseButton::Right)
            {
                cancelToolOrPlaceWalker(cell);
            }

            return;
        }

        if (const auto *released = std::get_if<PointerButtonReleased>(&event))
        {
            if (released->button == MouseButton::Left)
            {
                settle(
                    screenToCell(asPoint(released->position), camera));
            }

            return;
        }

        if (const auto *scrolled = std::get_if<PointerScrolled>(&event))
        {
            camera = zoomedAt(camera, input.pointer(), scrolled->vertical);
        }
    }

    void GridSink::begin(Cell cell, std::optional<BuildTool> tool)
    {
        drag.finish();
        pressedAt = std::nullopt;

        if (!tool.has_value())
        {
            return;
        }

        pressedAt = cell;

        if (dragsOut(*tool))
        {
            beginRoadDrag(cell);
        }
    }

    void GridSink::settle(Cell cell)
    {
        const auto from = pressedAt;
        const auto tool = overlay.tool();

        pressedAt = std::nullopt;

        if (drag.active())
        {
            endRoadDrag(cell);
            return;
        }

        if (!from.has_value() || !tool.has_value())
        {
            return;
        }

        placeOne(cell, *tool);
    }

    void GridSink::placeOne(Cell cell, BuildTool tool)
    {
        if (tool == BuildTool::Raze)
        {
            raze(cell);
            return;
        }

        placeBuilding(cell, *buildingKindOf(tool));
    }

    void GridSink::beginRoadDrag(Cell cell)
    {
        drag.begin(cell);
    }

    void GridSink::endRoadDrag(Cell cell)
    {
        drag.dragTo(cell);

        const auto tool = overlay.tool();
        const auto plan = planDrag(
            tool, drag.start(), drag.end(), extent, built);

        drag.finish();

        if (!plan.valid)
        {
            return;
        }

        const auto kind = tool.and_then(buildingKindOf);

        for (const auto on : plan.cells)
        {
            if (kind.has_value())
            {
                placeBuilding(on, *kind);
            }
            else
            {
                placePath(on);
            }
        }
    }

    void GridSink::placePath(Cell cell)
    {
        if (!canPave(cell, extent, paths, built))
        {
            return;
        }

        const auto entity = world.create();
        world.add<Cell>(entity, cell);
        world.add<Path>(entity, Path{});
        paths.insert(cell);

        state.money -= config.roadCost;
    }

    void GridSink::placeBuilding(Cell cell, BuildingKind kind)
    {
        const auto footprint = footprintOf(kind);

        if (!canPlace(cell, footprint, extent, paths, built))
        {
            return;
        }

        const auto entity = world.create();
        world.add<Cell>(entity, cell);

        Building put{.kind = kind};
        put.ticksUntilSpawn = config.spawnPeriodTicks - 1;
        put.ticksUntilDrain = config.drainPeriodTicks;
        put.ticksUntilRisk = config.riskPeriodTicks;
        world.add<Building>(entity, put);

        if (housesPeople(kind))
        {
            Coverage watered;
            watered.ticksLeft[serviceIndex(Service::Water)] =
                kCoverageFull;
            setCoverage(world, entity, watered);
        }

        (void)built.insert(cell, footprint);

        state.money -= config.costOf(kind);
    }

    void GridSink::cancelToolOrPlaceWalker(Cell cell)
    {
        const auto tool = overlay.tool();

        if (tool.has_value())
        {
            overlay.clearTool();
            return;
        }

        placeWalker(cell);
    }

    void GridSink::raze(Cell cell)
    {
        if (built.has(cell))
        {
            for (const auto entity : world.view<Building, Cell>())
            {
                const auto origin = world.get<Cell>(entity);
                const auto footprint =
                    footprintOf(world.get<Building>(entity).kind);

                if (covers(origin, footprint, cell))
                {
                    demolish(world, built, entity, extent, config);

                    state.money -= config.razeCost;
                    return;
                }
            }

            for (const auto entity : world.view<Ruin, Cell>())
            {
                const auto origin = world.get<Cell>(entity);
                const auto footprint =
                    footprintOf(world.get<Ruin>(entity).kind);

                if (covers(origin, footprint, cell))
                {
                    world.destroy(entity);
                    (void)built.erase(origin, footprint);
                    state.money -= config.razeCost;
                    return;
                }
            }

            return;
        }

        if (!paths.has(cell))
        {
            return;
        }

        for (const auto entity : world.view<Path, Cell>())
        {
            if (world.get<Cell>(entity) == cell)
            {
                world.destroy(entity);
                (void)paths.erase(cell);

                state.money -= config.razeCost;
                return;
            }
        }

    }

    void GridSink::placeWalker(Cell cell)
    {
        if (!paths.has(cell))
        {
            return;
        }

        const auto entity = world.create();
        world.add<Cell>(entity, cell);
        world.add<Walker>(entity, Walker{});
    }

}
