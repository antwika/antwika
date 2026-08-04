#pragma once

#include <optional>
#include <set>

#include <antwika/ecs/SystemScheduler.hpp>
#include <antwika/ecs/World.hpp>
#include <antwika/event/ITickEventSink.hpp>
#include <antwika/event/TickEvent.hpp>
#include <antwika/input/InputEvent.hpp>

#include "antwika/game/BuildingIndex.hpp"
#include "antwika/game/BuildTool.hpp"
#include "antwika/game/Camera.hpp"
#include "antwika/game/Cell.hpp"
#include "antwika/game/GameConfig.hpp"
#include "antwika/game/GameState.hpp"
#include "antwika/game/GridExtent.hpp"
#include "antwika/game/InputFold.hpp"
#include "antwika/game/PathIndex.hpp"
#include "antwika/game/RoadDrag.hpp"
#include "antwika/game/UiOverlay.hpp"
#include "antwika/game/WorldMapState.hpp"

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
     * | left press | place the selected tool's thing at the clicked cell |
     * | left drag | with the road tool, plan a run of road -- see below |
     * | left release | lay the planned run of road |
     * | right press | leave build mode, or place a walker -- see below |
     * | middle drag | pan the camera |
     * | scroll | zoom, keeping the cell under the cursor put |
     *
     * **The raze tool tears down instead of placing.** A left press
     * with it selected demolishes the building covering the clicked
     * cell -- any cell of the block -- through the very demolish() a
     * building lost to hunger goes through, so its occupants are
     * turned out identically; on a fire or on debris it clears the
     * ruin and frees the block, which is the only way burnt ground
     * ever comes back; on a road cell it takes the road up instead.
     * Each removal costs kRazeCost, charged only where something
     * actually came down, exactly as a placement is charged only
     * where something actually went up.
     * Razing is no more an event than placing is: a recording holds
     * the click, and a replay resolves it against the same city and
     * tears the same thing down. A right press with it selected puts
     * the palette down exactly as a building tool's does, since a
     * destructive mode somebody cannot back out of is a click away
     * from a building nobody meant to lose.
     *
     * **A road is dragged out rather than clicked one cell at a time.**
     * A left press with the road tool selected lays its own cell at once
     * and marks where a run of road starts; while the button is held the
     * cell under the pointer is where it would end, and planRoad() is
     * what says how it gets there; the release lays every cell of that
     * route in one go. Laying the pressed cell straight away rather than
     * waiting is what keeps a plain click the single-tile placement it
     * has always been -- and what keeps a recording with no release in
     * it, which is every recording written before this, laying exactly
     * what it always laid.
     *
     * **Only roads are dragged**, deliberately: a run of houses is not a
     * route, so a path search says nothing about where one would go, and
     * a building tool that dragged would need a rule of its own about
     * what a rectangle of blocks means. A building is therefore still
     * placed on the press and the press alone, and no drag begins.
     *
     * **A drag holds nothing still**, so a route is planned against a
     * city that goes on moving between the press and the release. It
     * used to pause the run for exactly that reason, and a city now runs
     * all the time unless a player has asked for a pause -- so what a
     * release lays is what the route came out as when it arrived, and a
     * drag can no longer resume a city somebody paused for themselves
     * because it never holds one.
     *
     * None of that is an event either. What a recording holds is the
     * press, the movements and the release, and a replay resolves them
     * against the same camera, the same palette and the same buildings
     * and arrives at the same route.
     *
     * **A right press means one of two things, and the palette decides
     * which.** While any tool is selected -- the road brush now
     * included -- it cancels: the palette is put down, selecting
     * nothing at all, and nothing is placed by that press. With
     * nothing selected, which is where a cancel leaves it, it drops a
     * walker on the path under the pointer.
     *
     * The road brush used to be the exception, dropping a walker
     * instead, which made the one armed mode a player could never
     * back out of the very one a session starts in.
     *
     * That split is what reconciles two claims on one button rather than
     * letting either silently replace the other, and cancelling reaches
     * a state of its own rather than falling back to Road: a fallback
     * would leave the palette holding a tool nobody chose and the next
     * left press laying a road for it. Nothing selected therefore places
     * nothing, previews nothing and holds no button down -- and since
     * that state drops walkers, cancelling twice is still cancelling
     * once.
     * A right press the toolbar covers cancels nothing, since what the
     * bar covers it covers from the grid.
     *
     * **Leaving build mode follows the rule placing follows, so it is
     * no more an event than laying a tile is.** What a recording holds is
     * the right press; a replay resolves it against the same selection
     * and arrives at the same one, exactly as a palette press is
     * regenerated rather than stored -- see UiOverlay and Events.hpp.
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
     * selected -- a road, one of the buildings, or nothing at all when
     * the palette has been put down. That selection is
     * simulation state for the same reason the camera is: a replay
     * carries the click and has to arrive at the same tool again, so
     * pressing a palette button is no more an event than pressing a zoom
     * button is.
     *
     * **It knows nothing about the placement ghost**, which is drawn
     * from input::PointerHintChannel on the render side. A replay does
     * not reproduce that channel, so a sink reading one would fold a
     * value into state that a replay cannot regenerate -- see
     * BuildGhost.
     *
     * Nothing at all is placed while no city is open. The mode gate this
     * sink is wrapped in already keeps a world-map click away from the
     * grid, and this is the second half of that: a city is put away the
     * moment the way-back key arrives, while the mode it staged does not
     * land until the tick boundary, so the events after it in that tick
     * would otherwise still be the grid's.
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
         * @param overlay Asked whether a click was the toolbar's, and
         * which tool is selected; a right press puts the palette down
         * in it, which is the one thing this sink writes there.
         * @param cities Asked whether a city is open at all; nothing is
         * placed, panned or zoomed while none is.
         * @param built Which cells hold a building.
         * @param drag Where a run of road starts and ends; written here
         * and read by whatever draws the preview.
         * @param state The bank each placement is paid out of; see
         * GameState::money for why spending is never refused.
         * @param config The costs and the periods a placement starts
         * with; copied, so no lifetime rule attaches to it.
         */
        GridSink(
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
            GameConfig config);

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
        void place(Cell cell, std::optional<BuildTool> tool);
        void placePath(Cell cell);
        void placeBuilding(Cell cell, BuildingKind kind);
        void raze(Cell cell);
        void cancelToolOrPlaceWalker(Cell cell);
        void placeWalker(Cell cell);
        void beginRoadDrag(Cell cell);
        void endRoadDrag(Cell cell);
        void act(const antwika::input::InputEvent &event);

        World &world;
        PathIndex &paths;
        Camera &camera;
        GridExtent extent;
        SystemScheduler &scheduler;
        const InputFold &input;
        UiOverlay &overlay;
        const WorldMapState &cities;

        // Which cells already hold a building.
        // PathIndex is the same note for roads.
        // Shared, now that a building can be demolished.
        // BuildingSystem is what clears a cell.
        // A note kept private here would never hear about it.
        BuildingIndex &built;

        // Where a run of road starts and where it has been taken.
        // Shared rather than private, since a preview is drawn from it.
        // Written here and nowhere else, inside the tick path.
        RoadDrag &drag;

        // The bank a placement is paid out of.
        // The reducer owns the other members; this sink spends money.
        // Two writers, and never of one field -- see GameState.
        GameState &state;
        GameConfig config;
    };

} // namespace antwika::game
