#include "antwika/game/GridSink.hpp"

#include <optional>
#include <variant>

#include <antwika/engine/Events.hpp>
#include <antwika/input/MouseButton.hpp>

#include "antwika/game/BuildTool.hpp"
#include "antwika/game/Building.hpp"
#include "antwika/game/Cell.hpp"
#include "antwika/game/IsoProjection.hpp"
#include "antwika/game/Footprint.hpp"
#include "antwika/game/Path.hpp"
#include "antwika/game/Placement.hpp"
#include "antwika/game/PointerReading.hpp"
#include "antwika/game/Walker.hpp"

namespace antwika::game
{

    using antwika::input::MouseButton;
    using antwika::input::PointerButtonPressed;
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
        BuildingIndex &built)
        : world(world),
          paths(paths),
          camera(camera),
          extent(extent),
          scheduler(scheduler),
          input(input),
          overlay(overlay),
          cities(cities),
          built(built)
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

        // A world-map click is not a click on any grid.
        // The mode gate says so a tick later; this says so at once.
        if (!cities.cityOpen())
        {
            return;
        }

        // Whatever the fold was just given, since it runs first.
        const auto &decoded = input.current();
        if (!decoded.has_value())
        {
            return;
        }

        act(*decoded);
    }

    void GridSink::act(const antwika::input::InputEvent &event)
    {
        // Whatever the toolbar covers, it covers from the grid too.
        // A movement is exempt, so a pan begun on the grid can cross it.
        if (overlay.pointerOverUi()
            && !std::holds_alternative<PointerMoved>(event))
        {
            return;
        }

        if (const auto *moved = std::get_if<PointerMoved>(&event))
        {
            // Only while the middle button is already down.
            // A press has then already established the pointer's place.
            // Folding a movement changes no button.
            // So asking now is the same as asking before it.
            if (input.state().mouse().isDown(MouseButton::Middle))
            {
                // The fold has already moved the pointer here.
                // So the distance is from where it was one event ago.
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
                place(cell, overlay.tool());
            }
            else if (pressed->button == MouseButton::Right)
            {
                cancelToolOrPlaceWalker(cell);
            }

            return;
        }

        if (const auto *scrolled = std::get_if<PointerScrolled>(&event))
        {
            camera = zoomedAt(camera, input.pointer(), scrolled->vertical);
        }
    }

    void GridSink::place(Cell cell, std::optional<BuildTool> tool)
    {
        // The palette is down, so a left press places nothing.
        // Not a road, which is what falling back to one would lay.
        // Somebody who has just cancelled has chosen to place nothing.
        if (!tool.has_value())
        {
            return;
        }

        // One decision, taken where the click is.
        // Rather than a button meaning one thing and the palette another.
        // The kind is worked out once here and handed on.
        // So nothing downstream asks again and disagrees.
        if (const auto kind = buildingKindOf(*tool))
        {
            placeBuilding(cell, *kind);
            return;
        }

        placePath(cell);
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
    }

    void GridSink::placeBuilding(Cell cell, BuildingKind kind)
    {
        const auto footprint = footprintOf(kind);

        // A cell holds one thing, and a road is a thing.
        // The note is kept in the index, not read out of the World.
        // The World hands out the last commit.
        // So two clicks in one tick would build twice on one cell.
        // That is the trap life::PointerToggleSink describes.
        if (!canPlace(cell, footprint, extent, paths, built))
        {
            return;
        }

        const auto entity = world.create();
        world.add<Cell>(entity, cell);
        world.add<Building>(entity, Building{.kind = kind});
        (void)built.insert(cell, footprint);
    }

    void GridSink::cancelToolOrPlaceWalker(Cell cell)
    {
        // One button, two meanings, and one place that decides which.
        // Splitting the decision between two sinks would take both.
        // UiSink runs first, and would clear the palette there.
        // This sink would then read the press and drop a walker too.
        // The cell is not consulted in the first arm at all.
        // Leaving build mode is about the palette, not about a cell.
        // See GridSink.hpp for the whole rule.
        const auto tool = overlay.tool();

        if (tool.has_value() && placesBuilding(*tool))
        {
            overlay.clearTool();
            return;
        }

        placeWalker(cell);
    }

    void GridSink::placeWalker(Cell cell)
    {
        // A walker goes *onto* a path, so bare ground takes none.
        if (!paths.has(cell))
        {
            return;
        }

        const auto entity = world.create();
        world.add<Cell>(entity, cell);
        world.add<Walker>(entity, Walker{});
    }

} // namespace antwika::game
