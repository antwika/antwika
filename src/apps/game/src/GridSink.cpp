#include "antwika/game/GridSink.hpp"

#include <array>
#include <variant>

#include <antwika/engine/Events.hpp>
#include <antwika/input/MouseButton.hpp>

#include "antwika/game/Cell.hpp"
#include "antwika/game/IsoProjection.hpp"
#include "antwika/game/Path.hpp"
#include "antwika/game/PointerReading.hpp"
#include "antwika/game/Walker.hpp"

namespace antwika::game
{

    using antwika::input::Key;
    using antwika::input::KeyPressed;
    using antwika::input::MouseButton;
    using antwika::input::PointerButtonPressed;
    using antwika::input::PointerMoved;
    using antwika::input::PointerScrolled;

    namespace
    {
        // The number keys, in the order the tools are declared.
        // A table rather than a switch, so the two orders are one fact.
        constexpr std::array<Key, 6> kToolKeys{
            Key::Digit1,
            Key::Digit2,
            Key::Digit3,
            Key::Digit4,
            Key::Digit5,
            Key::Digit6};
    } // namespace

    GridSink::GridSink(
        World &world,
        PathIndex &paths,
        BuildingIndex &buildings,
        Camera &camera,
        GridExtent extent,
        SystemScheduler &scheduler,
        const InputFold &input,
        const UiOverlay &overlay)
        : world(world),
          paths(paths),
          buildings(buildings),
          camera(camera),
          extent(extent),
          scheduler(scheduler),
          input(input),
          overlay(overlay)
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

        // Whatever the fold was just given, since it runs first.
        const auto &decoded = input.current();
        if (!decoded.has_value())
        {
            return;
        }

        act(*decoded);
    }

    BuildTool GridSink::tool() const noexcept
    {
        return selected;
    }

    void GridSink::act(const antwika::input::InputEvent &event)
    {
        if (const auto *key = std::get_if<KeyPressed>(&event))
        {
            // A repeat is a key still held, not a key chosen again.
            if (!key->repeat)
            {
                select(key->key);
            }

            return;
        }

        if (const auto *moved = std::get_if<PointerMoved>(&event))
        {
            // Only while the middle button is already down.
            // A press has then already established the pointer's place.
            // Folding a movement changes no button.
            // So asking now is the same as asking before it.
            //
            // A movement is never the toolbar's, even over it.
            // So a pan begun on the grid crosses the bar.
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

        // Whatever the toolbar covers, it covers from the grid too.
        if (overlay.pointerOverUi())
        {
            return;
        }

        if (const auto *pressed = std::get_if<PointerButtonPressed>(&event))
        {
            const auto cell =
                screenToCell(asPoint(pressed->position), camera);

            if (pressed->button == MouseButton::Left)
            {
                const auto kind = buildingFor(selected);

                if (kind.has_value())
                {
                    placeBuilding(cell, *kind);
                }
                else
                {
                    placePath(cell);
                }
            }
            else if (pressed->button == MouseButton::Right)
            {
                placeWalker(cell);
            }

            return;
        }

        if (const auto *scrolled = std::get_if<PointerScrolled>(&event))
        {
            camera = zoomedAt(camera, input.pointer(), scrolled->vertical);
        }
    }

    void GridSink::select(Key key)
    {
        for (std::size_t index = 0; index < kToolKeys.size(); ++index)
        {
            if (kToolKeys[index] == key)
            {
                selected = static_cast<BuildTool>(index);
                return;
            }
        }
    }

    void GridSink::placePath(Cell cell)
    {
        // A road may not be laid through a building.
        if (!extent.contains(cell) || paths.has(cell)
            || buildings.has(cell))
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
        // A building goes on bare ground: not on a road, not on another.
        if (!extent.contains(cell) || paths.has(cell)
            || buildings.has(cell))
        {
            return;
        }

        const auto entity = world.create();
        world.add<Cell>(entity, cell);
        world.add<Building>(entity, newlyBuilt(kind));
        buildings.insert(cell);
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
        world.add<Walker>(
            entity, newlySpawned(WalkerKind::Food, Direction::East));
    }

} // namespace antwika::game
