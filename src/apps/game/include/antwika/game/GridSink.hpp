#pragma once

#include <antwika/ecs/SystemScheduler.hpp>
#include <antwika/ecs/World.hpp>
#include <antwika/event/ITickEventSink.hpp>
#include <antwika/event/TickEvent.hpp>
#include <antwika/input/InputEvent.hpp>
#include <antwika/input/Key.hpp>

#include "antwika/game/Building.hpp"
#include "antwika/game/BuildingIndex.hpp"
#include "antwika/game/Camera.hpp"
#include "antwika/game/GridExtent.hpp"
#include "antwika/game/InputFold.hpp"
#include "antwika/game/PathIndex.hpp"
#include "antwika/game/UiOverlay.hpp"

namespace antwika::game
{

    using antwika::ecs::SystemScheduler;
    using antwika::ecs::World;
    using antwika::event::ITickEventSink;
    using antwika::event::TickEvent;

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
     * | left press | put down whatever the selected tool builds |
     * | right press | place a walker, only if that cell has a path |
     * | middle drag | pan the camera |
     * | scroll | zoom, keeping the cell under the cursor put |
     * | 1 .. 6 | select the road tool, or one of the five buildings |
     *
     * The selected tool is state of this sink's own, not a mode a menu
     * holds: it decides what a recorded click means, so a replay must
     * arrive at the same one, and it does because the key press that
     * chose it is recorded like every other input. A repeat is ignored,
     * since holding a number down is not choosing again.
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
         * @param world Path, walker and building entities are created
         * here.
         * @param paths Recorded into, and consulted before placing.
         * @param buildings Recorded into, and consulted before placing --
         * it is what stops one tick's two clicks stacking two buildings
         * on a cell the world has not shown yet.
         * @param camera Panned and zoomed; read by the renderer.
         * @param extent Bounds which cells a click may reach.
         * @param scheduler Run once per tick, after the commit.
         * @param input The folded input, holding the event being
         * handled; must be registered ahead of this sink.
         * @param overlay Asked whether a click was the toolbar's.
         */
        GridSink(
            World &world,
            PathIndex &paths,
            BuildingIndex &buildings,
            Camera &camera,
            GridExtent extent,
            SystemScheduler &scheduler,
            const InputFold &input,
            const UiOverlay &overlay);

        GridSink(const GridSink &) = delete;
        GridSink(GridSink &&) = delete;

        GridSink &operator=(const GridSink &) = delete;
        GridSink &operator=(GridSink &&) = delete;

        /**
         * @brief Apply a tick event.
         * @param event engine.tick commits the world and runs the
         * systems; an input.* event is acted on; anything else is
         * ignored.
         */
        void handle(const TickEvent &event) override;

        /**
         * @brief Get which tool a left click currently puts down.
         *
         * Exposed so that something drawing the session can say what is
         * selected. It is read-only on purpose: the only way to change it
         * is a key press, which is what a recording holds.
         *
         * @return The selected tool.
         */
        [[nodiscard]] BuildTool tool() const noexcept;

    private:
        void placePath(Cell cell);
        void placeBuilding(Cell cell, BuildingKind kind);
        void placeWalker(Cell cell);
        void select(antwika::input::Key key);
        void act(const antwika::input::InputEvent &event);

        World &world;
        PathIndex &paths;
        BuildingIndex &buildings;
        Camera &camera;
        GridExtent extent;
        SystemScheduler &scheduler;
        const InputFold &input;
        const UiOverlay &overlay;

        BuildTool selected = BuildTool::Path;
    };

} // namespace antwika::game
