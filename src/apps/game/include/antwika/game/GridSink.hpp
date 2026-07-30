#pragma once

#include <antwika/ecs/SystemScheduler.hpp>
#include <antwika/ecs/World.hpp>
#include <antwika/event/ITickEventSink.hpp>
#include <antwika/event/TickEvent.hpp>
#include <antwika/input/IInputEventCodec.hpp>
#include <antwika/input/InputEvent.hpp>
#include <antwika/input/InputState.hpp>

#include "antwika/game/Camera.hpp"
#include "antwika/game/GridExtent.hpp"
#include "antwika/game/PathIndex.hpp"
#include "antwika/game/UiOverlay.hpp"

namespace antwika::game
{

    using antwika::ecs::SystemScheduler;
    using antwika::ecs::World;
    using antwika::event::ITickEventSink;
    using antwika::event::TickEvent;
    using antwika::input::IInputEventCodec;
    using antwika::input::InputState;

    /**
     * @brief Turns this tick's input into paths, walkers and camera
     * movement, then runs the systems.
     *
     * **This is where input becomes application meaning, and it is
     * downstream of the recorder on purpose.** A replay stores the click
     * and regenerates the placement; if placing a path were its own event,
     * TickEventRecorder would write that alongside the click that caused
     * it and a replay would lay two tiles for one click. So this app
     * defines no event of its own for placing anything -- see Events.hpp.
     *
     * EngineLoop dispatches a tick's source events and *then* steps the
     * engine, so a tick's input all arrives before its engine.tick. That
     * ordering is what lets a scroll and a click in the same tick resolve
     * against the same, already-updated camera.
     *
     * | Gesture | Effect |
     * | --- | --- |
     * | left press | place a path at the clicked cell |
     * | right press | place a walker, only if that cell has a path |
     * | middle drag | pan the camera |
     * | scroll | zoom, keeping the cell under the cursor put |
     *
     * A press the toolbar is under never reaches the grid: what the UI
     * covers, it covers from the world too -- see UiOverlay. A movement
     * is exempt, so a pan that began on the grid carries on across the
     * bar rather than stopping dead under it.
     *
     * Middle-button drag rather than left, so that placement can stay on
     * the press: a left-drag pan would need a "moved more than N pixels,
     * so that was a drag" rule, which moves placement to the release and
     * invents a threshold nothing else here justifies.
     */
    class GridSink final : public ITickEventSink
    {
    public:
        /**
         * @brief Construct the sink over everything it drives.
         * @param world Path and walker entities are created here.
         * @param paths Recorded into, and consulted before placing.
         * @param camera Panned and zoomed; read by the renderer.
         * @param extent Bounds which cells a click may reach.
         * @param scheduler Run once per tick, after the commit.
         * @param codec Decodes the input events off the tick stream.
         * @param overlay Asked whether a click was the toolbar's.
         */
        GridSink(
            World &world,
            PathIndex &paths,
            Camera &camera,
            GridExtent extent,
            SystemScheduler &scheduler,
            const IInputEventCodec &codec,
            const UiOverlay &overlay);

        GridSink(const GridSink &) = delete;
        GridSink(GridSink &&) = delete;

        GridSink &operator=(const GridSink &) = delete;
        GridSink &operator=(GridSink &&) = delete;

        /**
         * @brief Apply a tick event.
         * @param event engine.tick commits the world and runs the
         * systems; an input.* event is folded and acted on; anything else
         * is ignored.
         * @throws antwika::input::InputError If an input.* event carries a
         * payload of the wrong shape -- raised by the codec, since the
         * wire format is its to police.
         */
        void handle(const TickEvent &event) override;

    private:
        void placePath(Cell cell);
        void placeWalker(Cell cell);
        void act(const antwika::input::InputEvent &event, Point previous,
                 bool wasDragging);

        World &world;
        PathIndex &paths;
        Camera &camera;
        GridExtent extent;
        SystemScheduler &scheduler;
        const IInputEventCodec &codec;
        const UiOverlay &overlay;

        // Held here rather than above the recorder.
        // A replay then regenerates what a click is read against.
        InputState state;
    };

} // namespace antwika::game
