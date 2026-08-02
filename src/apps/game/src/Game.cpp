#include "antwika/game/Game.hpp"

#include <antwika/ecs/SystemScheduler.hpp>
#include <antwika/ecs/World.hpp>
#include <antwika/engine/Engine.hpp>
#include <antwika/engine/StopSignal.hpp>
#include <antwika/event/Event.hpp>
#include <antwika/event/EventDispatcher.hpp>
#include <antwika/event/TickedEventDispatcher.hpp>
#include <antwika/i18n/Locale.hpp>
#include <antwika/i18n/Translator.hpp>
#include <antwika/log/Level.hpp>
#include <antwika/simulation/EngineLoop.hpp>

#include "antwika/game/Events.hpp"
#include "antwika/game/GameStateReducer.hpp"
#include "antwika/game/BuildingSystem.hpp"
#include "antwika/game/GridSink.hpp"
#include "antwika/game/HaulingSystem.hpp"
#include "antwika/game/InputFold.hpp"
#include "antwika/game/LiveGrid.hpp"
#include "antwika/game/MarketSystem.hpp"
#include "antwika/game/MainMenuScene.hpp"
#include "antwika/game/MainMenuSink.hpp"
#include "antwika/game/MenuModalScene.hpp"
#include "antwika/game/ModeGatedSink.hpp"
#include "antwika/game/PauseGatedSystem.hpp"
#include "antwika/game/PauseState.hpp"
#include "antwika/game/ProductionSystem.hpp"
#include "antwika/game/SaveGameFile.hpp"
#include "antwika/game/SaveLoadScene.hpp"
#include "antwika/game/SaveLoadSink.hpp"
#include "antwika/game/SaveLoadState.hpp"
#include "antwika/game/SceneSnapshot.hpp"
#include "antwika/game/SessionGatedSystem.hpp"
#include "antwika/game/SessionStore.hpp"
#include "antwika/game/SpawnSystem.hpp"
#include "antwika/game/BuildingKind.hpp"
#include "antwika/game/Toolbar.hpp"
#include "antwika/game/UiSink.hpp"
#include "antwika/game/WalkerSystem.hpp"
#include "antwika/game/WorldMapSink.hpp"

using antwika::ecs::SystemScheduler;
using antwika::ecs::World;
using antwika::engine::Engine;
using antwika::engine::StopSignal;
using antwika::event::Event;
using antwika::event::EventDispatcher;
using antwika::event::TickedEventDispatcher;
using antwika::log::Level;
using antwika::simulation::EngineLoop;

namespace antwika::game
{

    Game::Game(IEngine &engine, ILogger &logger)
        : engine(engine), logger(logger)
    {
    }

    void Game::run()
    {
        logger.log(Level::Info, "Running Antwika Game");
        engine.start();
    }

    GameSummary bootstrap(const GameConfig &config)
    {
        ILogger &logger = config.logger;
        Camera &camera = config.camera;
        PathIndex &paths = config.paths;

        // Fixed in source either way -- see GameConfig::translator.
        const antwika::i18n::Translator ownTranslator{
            antwika::i18n::kDefaultLocale};
        const antwika::i18n::Translator &translator =
            config.translator.has_value() ? config.translator->get()
                                          : ownTranslator;

        EventDispatcher dispatcher({config.eventSink});

        World world(logger);

        AppModeState &mode = config.mode;

        SystemScheduler scheduler;
        WalkerSystem walkerSystem(paths, config.extent);
        SpawnSystem spawnSystem(paths);
        BuildingSystem buildingSystem(config.built);

        // The walkers stop with the session, never with the screen.
        // Only that one system stops.
        // The tick, the commit and every observer still run.
        // So the menu is drawn and the run is still paced.
        // A city goes on running while its player reads the map.
        // What stops one is a player asking -- see PauseState.
        SessionGatedSystem gatedWalkers(walkerSystem, mode);

        // The buildings run with them, and on the same terms.
        SessionGatedSystem gatedSpawns(spawnSystem, mode);

        // And so does the economy.
        SessionGatedSystem gatedBuildings(buildingSystem, mode);

        // What a pause stops, and all it stops.
        // These three are what make the city move on its own.
        // Everything else carries on.
        // The tick, the commit, the renderer, the bar and the camera.
        // So a paused city can still be panned over and built on.
        // That is the product decision here.
        // This is a build pause rather than a freeze.
        // The same call apps/life makes about drawing on a paused board.
        // A run is paused only where a player has asked for one.
        // A city coming up runs, as it does behind a menu or a map.
        // The way in and out is the bar's pause button, both ways.
        // Owned by the caller, as the camera and the mode are.
        // A renderer built before this call has to read it too.
        PauseState &pause = config.pause;
        PauseGatedSystem pausedWalkers(gatedWalkers, pause);
        PauseGatedSystem pausedBuildings(gatedBuildings, pause);
        PauseGatedSystem pausedSpawns(gatedSpawns, pause);

        const auto walkPhase = scheduler.createPhase("walk");
        scheduler.addSystem(walkPhase, pausedWalkers);

        // After the walk, so a delivery sees this tick's cells.
        scheduler.addSystem(walkPhase, pausedBuildings);

        // After the walk, so a walker made this tick sets off next one.
        // Both stage into the same buffer, so neither sees the other.
        // Last, so a building demolished this tick is not re-let now.
        scheduler.addSystem(walkPhase, pausedSpawns);

        // The goods chain, gated exactly as the walk is.
        // A city runs while its player reads the world map.
        // And it stops only where a player asked.
        // So both gates, in that order.
        ProductionSystem productionSystem;
        HaulingSystem haulingSystem(paths, config.extent);
        MarketSystem marketSystem(paths, config.extent);

        SessionGatedSystem gatedProduction(productionSystem, mode);
        SessionGatedSystem gatedHauling(haulingSystem, mode);
        SessionGatedSystem gatedMarkets(marketSystem, mode);

        PauseGatedSystem pausedProduction(gatedProduction, pause);
        PauseGatedSystem pausedHauling(gatedHauling, pause);
        PauseGatedSystem pausedMarkets(gatedMarkets, pause);

        // Two phases rather than one, and the split is load bearing.
        // A phase is where the World's buffers swap.
        // So two systems in one both read what the last swap left.
        // And both write a whole Building back.
        // So a batch added and a cart-load taken is not arithmetic.
        // The later write silently undoes the earlier.
        // The commit between these two is what makes it arithmetic.
        const auto producePhase = scheduler.createPhase("produce");
        scheduler.addSystem(producePhase, pausedProduction);

        // Hauling and the markets share one.
        // Nothing they write overlaps.
        // A cart is loaded out of a producer.
        // A buyer is loaded out of a storehouse.
        // And no building is both.
        const auto haulPhase = scheduler.createPhase("haul");
        scheduler.addSystem(haulPhase, pausedHauling);
        scheduler.addSystem(haulPhase, pausedMarkets);

        // A phase of its own.
        // A renderer then sees the generation this walk produced.
        const auto observePhase = scheduler.createPhase("observe");
        for (auto &observer : config.observers)
        {
            scheduler.addSystem(observePhase, observer.get());
        }

        GameState state;
        GameStateReducer reducer(state);

        // A run with no toolbar still needs something the grid can ask.
        // An overlay nothing writes covers nothing.
        // So every click is the world's, which is what that means.
        UiOverlay noToolbar;
        const bool hasToolbar = config.overlay.has_value();
        UiOverlay &ui = hasToolbar ? config.overlay->get() : noToolbar;

        // The menu's own picture, never the toolbar's.
        // The two belong to different modes.
        // Neither may overwrite the other's.
        UiOverlay noMenu;
        UiOverlay &menuUi = config.menuOverlay.has_value()
                                ? config.menuOverlay->get()
                                : noMenu;

        // A run with no world map still has one city, permanently open.
        // So the grid is not gated off by a map that is not there.
        // The world itself is empty, and nothing draws it.
        WorldMapState oneCity{WorldMap{}};
        WorldMapState &cities =
            config.world.has_value() ? config.world->get() : oneCity;

        // A run with nobody to show a preview to still drags roads out.
        // So one is made here when no caller has offered one.
        RoadDrag ownDrag;
        RoadDrag &drag =
            config.drag.has_value() ? config.drag->get() : ownDrag;

        const Toolbar toolbar(translator);
        const MenuModalScene menuModal(translator);
        InputFold input(config.codec);
        UiSink uiSink(
            camera,
            ui,
            input,
            toolbar,
            pause,
            mode,
            drag,
            menuModal,
            camera);
        GridSink gridSink(
            world,
            paths,
            camera,
            config.extent,
            scheduler,
            input,
            ui,
            cities,
            config.built,
            drag);

        // The four that are swapped together, named together.
        // A city is opened by putting its contents into these.
        const LiveGrid live{
            .world = world,
            .paths = paths,
            .built = config.built,
            .camera = camera};

        WorldMapSink worldSink(
            cities, mode, live, input, config.canvas);
        StopSignal stopSignal;

        const MainMenuScene menuScene(translator);
        MainMenuSink menuSink(
            mode, menuUi, input, menuScene, stopSignal);

        // The save screen's own picture, never the menu's or the bar's.
        // Three modes, three overlays, for the reason the menu has one.
        UiOverlay noSaveScreen;
        UiOverlay &saveUi = config.saveOverlay.has_value()
                                ? config.saveOverlay->get()
                                : noSaveScreen;

        SessionStore session(
            world,
            paths,
            config.built,
            camera,
            state,
            config.extent,
            config.seed);

        // Restored before the first tick.
        // Through the very store the Load button uses.
        // So the two cannot come out differently.
        if (config.start.has_value())
        {
            session.restore(*config.start);
        }

        SaveLoadState saveState(config.saves);
        const SaveLoadScene saveScene(translator);
        SaveLoadSink saveSink(
            saveState,
            mode,
            saveUi,
            input,
            saveScene,
            session,
            config.saveDirectory);

        // Gated on the mode rather than checking one themselves.
        // What a mode changes is what a click means.
        // So engine.tick still reaches both -- see ModeGatedSink.
        ModeGatedSink playingUi(uiSink, mode, AppMode::CityMap);
        ModeGatedSink playingGrid(gridSink, mode, AppMode::CityMap);

        // The fold is first.
        // What it holds is the event the sinks after it are given now.
        // It is also the only thing that clears an edge.
        // So the tick boundary is one rule in one place.
        // GridSink runs the scheduler on engine.tick.
        // So anything that must show in this frame is folded before it.
        // UiSink still comes before it.
        // So a press is resolved against the bar before the grid sees it.
        // And the picture is described before the renderer paints it.
        // The mode is committed straight after the fold.
        // A change staged last tick lands before anything gated reads it.
        // MainMenuSink is before the grid's for the reason UiSink is.
        // A press is resolved against what is on screen first.
        // WorldMapSink is between the bar and the grid for the same one.
        // A press that opens a city must not also build in it.
        // SaveLoadSink is beside MainMenuSink for the same reason.
        // Both gate themselves, and both own a whole screen.
        std::vector<std::reference_wrapper<ITickEventSink>> timedSinks{
            input, mode, reducer, menuSink, saveSink};

        // Registered only when there is somewhere to put the picture.
        // Otherwise the bar is described against a zero canvas.
        // Which no click can hit, so nothing is ever hovered or pressed.
        // "No toolbar" then means no toolbar, not an unhittable one.
        if (hasToolbar)
        {
            timedSinks.push_back(playingUi);
        }

        // Registered only when there is a world to map.
        // A run with one city has nowhere to go back to.
        // The way-back key would then put its only grid away.
        if (config.world.has_value())
        {
            timedSinks.push_back(worldSink);
        }

        timedSinks.push_back(playingGrid);
        timedSinks.push_back(stopSignal);

        if (config.replayRecorder.has_value())
        {
            timedSinks.push_back(config.replayRecorder->get());
        }
        TickedEventDispatcher tickedDispatcher(dispatcher, timedSinks);

        Engine engine(logger, tickedDispatcher);
        Game game(engine, logger);
        game.run();

        EngineLoop loop(engine, tickedDispatcher, config.inputSource);
        loop.run(stopSignal, config.maxTicks);

        // Taken while the World is still here.
        // A save is read out of it rather than out of the summary.
        antwika::game::saveGameFileIfNamed(
            session.take(), config.savePath);

        const auto frame =
            snapshotOf(world, paths, camera, config.extent);

        // Every branch left on the excluded line is the allocator's.
        // Two are the throw edges of copying the two vectors.
        // The last is a heap branch nothing here is large enough to take.
        return GameSummary{ // GCOVR_EXCL_LINE
            .state = state,
            .paths = frame.paths,
            .walkers = walkerViewsOf(world),
            .buildings = buildingViewsOf(world),
            .camera = camera};
        // The excluded line is the local summary's unwind destructor.
        // Nothing between its construction and the return throws.
    } // GCOVR_EXCL_LINE

    void printSummary(std::ostream &out, const GameSummary &summary)
    {
        out << "Final state: ticksProcessed="
            << summary.state.ticksProcessed
            << " score=" << summary.state.score << '\n';
        out << "Paths laid: " << summary.paths.size() << '\n';
        out << "Walkers: " << summary.walkers.size() << '\n';

        for (const auto &walker : summary.walkers)
        {
            out << "  at (" << walker.at.x << ", " << walker.at.y
                << ") facing " << directionIndex(walker.facing) << '\n';
        }

        out << "Buildings: " << summary.buildings.size() << '\n';

        for (const auto &building : summary.buildings)
        {
            out << "  " << buildingKindName(building.kind) << " at ("
                << building.at.x << ", " << building.at.y << ")\n";
        }

        out << "Camera: pan (" << summary.camera.pan().x << ", "
            << summary.camera.pan().y << ") zoom "
            << summary.camera.zoomLevel() << '\n';
    }

} // namespace antwika::game
