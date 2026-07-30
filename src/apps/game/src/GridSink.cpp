#include "antwika/game/GridSink.hpp"

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
        const UiOverlay &overlay)
        : world(world),
          paths(paths),
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
                placePath(cell);
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

    void GridSink::placePath(Cell cell)
    {
        if (!extent.contains(cell) || paths.has(cell))
        {
            return;
        }

        const auto entity = world.create();
        world.add<Cell>(entity, cell);
        world.add<Path>(entity, Path{});
        paths.insert(cell);
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
