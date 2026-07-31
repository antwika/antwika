#pragma once

#include <optional>
#include <set>

#include <antwika/ecs/Entity.hpp>
#include <antwika/ecs/SystemScheduler.hpp>
#include <antwika/ecs/World.hpp>
#include <antwika/event/ITickEventSink.hpp>
#include <antwika/event/TickEvent.hpp>
#include <antwika/input/InputEvent.hpp>

#include "antwika/game/BuildTool.hpp"
#include "antwika/game/Camera.hpp"
#include "antwika/game/Cell.hpp"
#include "antwika/game/GridExtent.hpp"
#include "antwika/game/InputFold.hpp"
#include "antwika/game/PathIndex.hpp"
#include "antwika/game/UiOverlay.hpp"

namespace antwika::game
{

    using antwika::ecs::Entity;
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
     * | left press | place the selected tool's thing at the clicked cell |
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
     *
     * What a left press places is whatever UiOverlay says the palette has
     * selected -- a road or one of the buildings. That selection is
     * simulation state for the same reason the camera is: a replay
     * carries the click and has to arrive at the same tool again, so
     * pressing a palette button is no more an event than pressing a zoom
     * button is.
     *
     * Every input event also restates the BuildGhost: where the selected
     * tool would land if it were clicked now. It is worked out here
     * rather than by the renderer because it is a function of the camera,
     * and it goes into the World because that is the only thing a
     * SceneSnapshot is taken from. **It follows the pointer only on a
     * click, a wheel or a key**, since input::IdleMotionSource holds back
     * movement while no button is held -- see BuildGhost.
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
         * @param input The folded input, holding the event being
         * handled; must be registered ahead of this sink.
         * @param overlay Asked whether a click was the toolbar's.
         */
        GridSink(
            World &world,
            PathIndex &paths,
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

    private:
        void place(Cell cell, BuildTool tool);
        void placePath(Cell cell);
        void placeBuilding(Cell cell, BuildTool tool);
        void placeWalker(Cell cell);
        void act(const antwika::input::InputEvent &event);
        void updateGhost();

        World &world;
        PathIndex &paths;
        Camera &camera;
        GridExtent extent;
        SystemScheduler &scheduler;
        const InputFold &input;
        const UiOverlay &overlay;

        // Which cells already hold a building.
        // PathIndex is the same note for roads.
        // This one is private: nothing outside asks for it.
        std::set<Cell> built;

        // The one entity the ghost's component lives on.
        // Made when the first input arrives, not in the constructor.
        // A run that takes no input then creates nothing.
        std::optional<Entity> ghost;
    };

} // namespace antwika::game
