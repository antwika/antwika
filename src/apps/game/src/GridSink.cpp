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
        const IInputEventCodec &codec,
        const UiOverlay &overlay)
        : world(world),
          paths(paths),
          camera(camera),
          extent(extent),
          scheduler(scheduler),
          codec(codec),
          overlay(overlay)
    {
    }

    void GridSink::handle(const TickEvent &event)
    {
        if (event.event.name == antwika::engine::events::kTick)
        {
            world.commit();
            scheduler.run(world, event.tick);

            // After the systems, so nothing runs against cleared edges.
            state.beginTick();
            return;
        }

        const auto decoded = codec.decode(event.event);
        if (!decoded.has_value())
        {
            return;
        }

        // Read what a drag needs before folding this event in.
        // Applying it is what moves the pointer.
        const auto previous = asPoint(state.mouse().position());
        const auto wasDragging = state.mouse().isDown(MouseButton::Middle);

        state.apply(*decoded);

        act(*decoded, previous, wasDragging);
    }

    void GridSink::act(
        const antwika::input::InputEvent &event,
        Point previous,
        bool wasDragging)
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
            if (wasDragging)
            {
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
            camera = zoomedAt(
                camera,
                asPoint(state.mouse().position()),
                scrolled->vertical);
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
