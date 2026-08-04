#include "antwika/game/Game.hpp"

#include <antwika/ecs/SystemScheduler.hpp>
#include <antwika/ecs/World.hpp>
#include <antwika/engine/Engine.hpp>
#include <antwika/engine/StopSignal.hpp>
#include <antwika/event/Event.hpp>
#include <antwika/event/EventDispatcher.hpp>
#include <antwika/event/TickedEventDispatcher.hpp>
#include <antwika/i18n/Locale.hpp>
#include <antwika/log/Level.hpp>
#include <antwika/simulation/EngineLoop.hpp>

#include "antwika/game/BindingSink.hpp"
#include "antwika/game/BuildingKind.hpp"
#include "antwika/game/BuildingSystem.hpp"
#include "antwika/game/CityRatings.hpp"
#include "antwika/game/CoverageSystem.hpp"
#include "antwika/game/Desirability.hpp"
#include "antwika/game/DesirabilitySystem.hpp"
#include "antwika/game/Events.hpp"
#include "antwika/game/GameStateReducer.hpp"
#include "antwika/game/GridSink.hpp"
#include "antwika/game/HaulingSystem.hpp"
#include "antwika/game/HotkeySink.hpp"
#include "antwika/game/HousingLevel.hpp"
#include "antwika/game/HousingSystem.hpp"
#include "antwika/game/InputFold.hpp"
#include "antwika/game/LocaleState.hpp"
#include "antwika/game/LabourDispatchSystem.hpp"
#include "antwika/game/StaffingSystem.hpp"
#include "antwika/game/LiveGrid.hpp"
#include "antwika/game/MainMenuScene.hpp"
#include "antwika/game/MainMenuSink.hpp"
#include "antwika/game/SupplySystem.hpp"
#include "antwika/game/MenuCommands.hpp"
#include "antwika/game/MenuModalScene.hpp"
#include "antwika/game/Messages.hpp"
#include "antwika/game/ModeGatedSink.hpp"
#include "antwika/game/OptionsFile.hpp"
#include "antwika/game/OptionsScene.hpp"
#include "antwika/game/OptionsState.hpp"
#include "antwika/game/PauseGatedSystem.hpp"
#include "antwika/game/PauseState.hpp"
#include "antwika/game/PopulationSystem.hpp"
#include "antwika/game/ProductionSystem.hpp"
#include "antwika/game/RatingsSystem.hpp"
#include "antwika/game/Ruin.hpp"
#include "antwika/game/RuinSystem.hpp"
#include "antwika/game/SaveGameFile.hpp"
#include "antwika/game/SaveLoadScene.hpp"
#include "antwika/game/SaveLoadSink.hpp"
#include "antwika/game/SaveLoadState.hpp"
#include "antwika/game/SceneSnapshot.hpp"
#include "antwika/game/Service.hpp"
#include "antwika/game/SessionGatedSystem.hpp"
#include "antwika/game/SessionStore.hpp"
#include "antwika/game/SpawnSystem.hpp"
#include "antwika/game/Toolbar.hpp"
#include "antwika/game/UiSink.hpp"
#include "antwika/game/ViewCommands.hpp"
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

    GameSummary bootstrap(const GameWiring &wiring)
    {
        ILogger &logger = wiring.logger;
        Camera &camera = wiring.camera;
        PathIndex &paths = wiring.paths;

        // Fixed in source either way -- see GameWiring::translator.
        // Seeded from the injected translator when a test gave one.
        // A Translator holds a locale and nothing else, so this is the
        // whole of what such a test was saying -- see LocaleState.
        LocaleState localeState(
            wiring.translator.has_value()
                ? wiring.translator->get().locale()
                : antwika::i18n::kDefaultLocale);
        const Translator &translator = localeState.translator();

        EventDispatcher dispatcher({wiring.eventSink});

        World world(logger);

        AppModeState &mode = wiring.mode;

        SystemScheduler scheduler;
        WalkerSystem walkerSystem(paths, wiring.built, wiring.extent);
        SpawnSystem spawnSystem(paths, wiring.config);
        BuildingSystem buildingSystem(
            wiring.built, wiring.extent, wiring.config);

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
        PauseState &pause = wiring.pause;
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

        // A phase of its own, after the walk, for the fires.
        // WalkerSystem writes Ruin when a fireman arrives.
        // This writes Ruin burning one down and tasking the idle.
        // Two writers of one component in one phase is the trap.
        // The phase list exists to avoid, so the commit sits between.
        // Gated as the walk is: a fire burns behind the world map.
        // And holds still only where a player asked.
        RuinSystem ruinSystem;

        SessionGatedSystem gatedRuins(ruinSystem, mode);
        PauseGatedSystem pausedRuins(gatedRuins, pause);

        const auto burnPhase = scheduler.createPhase("burn");
        scheduler.addSystem(burnPhase, pausedRuins);

        // A phase of its own, after the walk and before the observers.
        // So it sees where this tick left every walker.
        // And a renderer sees the coverage that produced.
        // Both gates, in the order every other system takes them.
        // A city serves itself while its player reads the world map.
        // And stops only where a player asked -- see PauseState.
        CoverageSystem coverageSystem;

        // The caller's where there is one, for the drag's reason.
        // A renderer built before this call paints the view off it.
        DesirabilityField ownDesirability;
        DesirabilityField &desirability = wiring.desirability.has_value()
            ? wiring.desirability->get()
            : ownDesirability;

        DesirabilitySystem desirabilitySystem(desirability, wiring.extent);

        SessionGatedSystem gatedCoverage(coverageSystem, mode);
        SessionGatedSystem gatedDesirability(desirabilitySystem, mode);
        PauseGatedSystem pausedCoverage(gatedCoverage, pause);
        PauseGatedSystem pausedDesirability(gatedDesirability, pause);

        const auto servePhase = scheduler.createPhase("serve");
        scheduler.addSystem(servePhase, pausedCoverage);
        scheduler.addSystem(servePhase, pausedDesirability);

        // The goods chain, gated exactly as the walk is.
        // A city runs while its player reads the world map.
        // And it stops only where a player asked.
        // So both gates, in that order.
        ProductionSystem productionSystem(wiring.config);
        HaulingSystem haulingSystem(paths, wiring.extent);
        SupplySystem supplySystem(
            paths, wiring.extent, wiring.config);

        SessionGatedSystem gatedProduction(productionSystem, mode);
        SessionGatedSystem gatedHauling(haulingSystem, mode);
        SessionGatedSystem gatedSupply(supplySystem, mode);

        PauseGatedSystem pausedProduction(gatedProduction, pause);
        PauseGatedSystem pausedHauling(gatedHauling, pause);
        PauseGatedSystem pausedSupply(gatedSupply, pause);

        // Two phases rather than one, and the split is load bearing.
        // A phase is where the World's buffers swap.
        // So two systems in one both read what the last swap left.
        // And both write a whole Building back.
        // So a batch added and a cart-load taken is not arithmetic.
        // The later write silently undoes the earlier.
        // The commit between these two is what makes it arithmetic.
        const auto producePhase = scheduler.createPhase("produce");
        scheduler.addSystem(producePhase, pausedProduction);

        // Hauling has one to itself.
        // It writes every producer, loading a cart out of each.
        const auto haulPhase = scheduler.createPhase("haul");
        scheduler.addSystem(haulPhase, pausedHauling);

        // And the supply of what a building cannot make has another.
        // These two used to share the haul phase.
        // Which was safe while no building was both ends of it.
        // A cart is loaded out of a producer.
        // A buyer is loaded out of a storehouse.
        // A workshop is both: pottery out, clay in.
        // So the two would write one Building in one phase.
        // And the later write would silently undo the earlier.
        // The commit between them is what makes it arithmetic.
        const auto supplyPhase = scheduler.createPhase("supply");
        scheduler.addSystem(supplyPhase, pausedSupply);

        // After the goods have moved and before anything is drawn.
        // So a house is judged on the shelves this tick filled.
        // The field it judges its ground by is the serve phase's.
        // Which is this tick's too, two phases earlier.
        // Both gates, in the order every other system takes them.
        HousingSystem housingSystem(desirability, wiring.config);

        SessionGatedSystem gatedHousing(housingSystem, mode);
        PauseGatedSystem pausedHousing(gatedHousing, pause);

        const auto settlePhase = scheduler.createPhase("settle");
        scheduler.addSystem(settlePhase, pausedHousing);

        // A phase of its own rather than a second entry in "settle".
        // Because a phase is where the World's buffers swap.
        // HousingSystem writes a whole Household back.
        // And so does PopulationSystem.
        // Two of them in one phase both read what the last swap left.
        // So the tier one wrote and the occupancy the other wrote.
        // Could not both survive the tick they were written in.
        // The later write would silently undo the earlier one.
        // Some of the time, which is the worst kind.
        // The commit between these two is what makes them sequential.
        //
        // Labour shares the phase, and that is safe on its own terms.
        // It writes Workforce, which nothing else writes.
        // And it reads the population as the settle phase left it.
        // Which is one tick behind the one being counted here.
        // A person arriving is employable from the following tick.
        // Both gates, in the order every other system takes them.
        PopulationSystem populationSystem(
            paths,
            wiring.built,
            desirability,
            wiring.extent,
            wiring.config);

        SessionGatedSystem gatedPopulation(populationSystem, mode);
        PauseGatedSystem pausedPopulation(gatedPopulation, pause);

        const auto populatePhase = scheduler.createPhase("populate");
        scheduler.addSystem(populatePhase, pausedPopulation);

        // Labour walks now, and its two systems have a phase each.
        // Staffing writes Staff, Employment and the labourers.
        // In a phase of its own after the walk.
        // So it moves people off where this tick left every labourer.
        // Dispatch writes Employment and Building too.
        // Two writers of one component in one phase is the trap.
        // The phase list exists to avoid it, so this runs after.
        // A person who arrived this tick is employable next one.
        StaffingSystem staffingSystem(wiring.config);
        LabourDispatchSystem dispatchSystem(paths, wiring.config);

        SessionGatedSystem gatedStaffing(staffingSystem, mode);
        SessionGatedSystem gatedDispatch(dispatchSystem, mode);
        PauseGatedSystem pausedStaffing(gatedStaffing, pause);
        PauseGatedSystem pausedDispatch(gatedDispatch, pause);

        const auto staffPhase = scheduler.createPhase("staff");
        scheduler.addSystem(staffPhase, pausedStaffing);

        const auto hirePhase = scheduler.createPhase("hire");
        scheduler.addSystem(hirePhase, pausedDispatch);

        // A phase of its own.
        // A renderer then sees the generation this walk produced.
        const auto observePhase = scheduler.createPhase("observe");

        // Ahead of the observers, and in this phase rather than the last.
        // So it rates the city this tick left rather than the one before.
        // Everything that moves a person or a job has committed by now.
        CityRatings ratings;
        RatingsSystem ratingsSystem(ratings);

        SessionGatedSystem gatedRatings(ratingsSystem, mode);
        PauseGatedSystem pausedRatings(gatedRatings, pause);

        scheduler.addSystem(observePhase, pausedRatings);

        for (auto &observer : wiring.observers)
        {
            scheduler.addSystem(observePhase, observer.get());
        }

        GameState state;
        state.money = wiring.config.startingMoney;
        GameStateReducer reducer(state);

        // A run with no toolbar still needs something the grid can ask.
        // An overlay nothing writes covers nothing.
        // So every click is the world's, which is what that means.
        UiOverlay noToolbar;
        const bool hasToolbar = wiring.overlay.has_value();
        UiOverlay &ui = hasToolbar ? wiring.overlay->get() : noToolbar;

        // The menu's own picture, never the toolbar's.
        // The two belong to different modes.
        // Neither may overwrite the other's.
        UiOverlay noMenu;
        UiOverlay &menuUi = wiring.menuOverlay.has_value()
                                ? wiring.menuOverlay->get()
                                : noMenu;

        // A run with no world map still has one city, permanently open.
        // So the grid is not gated off by a map that is not there.
        // The world itself is empty, and nothing draws it.
        WorldMapState oneCity{WorldMap{}};
        WorldMapState &cities =
            wiring.world.has_value() ? wiring.world->get() : oneCity;

        // A run with nobody to show a preview to still drags roads out.
        // So one is made here when no caller has offered one.
        RoadDrag ownDrag;
        RoadDrag &drag =
            wiring.drag.has_value() ? wiring.drag->get() : ownDrag;

        // And one with nobody to paint an overlay for still runs.
        // So one of those is made here on exactly those terms.
        MapViewState ownView;
        MapViewState &view =
            wiring.view.has_value() ? wiring.view->get() : ownView;

        // The four that are swapped together, named together.
        // A city is opened by putting its contents into these.
        // Declared here rather than beside the world-map sink.
        // The game menu puts a city away through the same four.
        const LiveGrid live{
            .world = world,
            .paths = paths,
            .built = wiring.built,
            .camera = camera};

        SessionStore session(
            world,
            paths,
            wiring.built,
            camera,
            state,
            wiring.extent,
            wiring.seed);

        // What the top bar's game menu does, written once.
        // The menu modal's own way out goes through it too.
        // So leaving for the main menu is one transition, not two.
        MenuCommands commands(
            mode, session, cities, live, camera, wiring.config);

        // What the bar's buttons and the bound keys both ask for.
        // Stated once so the two routes to a zoom cannot drift.
        ViewCommands viewCommands(camera, pause, camera);

        const Toolbar toolbar(translator);
        const MenuModalScene menuModal(translator);
        InputFold input(wiring.codec);
        UiSink uiSink(
            camera,
            ui,
            input,
            toolbar,
            pause,
            view,
            commands,
            drag,
            menuModal,
            viewCommands,
            ratings,
            state);
        GridSink gridSink(
            world,
            paths,
            camera,
            wiring.extent,
            scheduler,
            input,
            ui,
            cities,
            wiring.built,
            drag,
            state,
            wiring.config);

        WorldMapSink worldSink(
            cities, mode, live, input, wiring.canvas);
        StopSignal stopSignal;

        // Owned here rather than by the caller.
        // Unlike the camera and the mode, nothing outside reads it.
        // The options screen is drawn off the menu's own overlay.
        // By the menu's own scene.
        // So a renderer never has to know that it exists.
        OptionsState options;
        BindingSink bindingSink(options);

        const MainMenuScene menuScene(translator);
        const OptionsScene optionsScene(
            translator, localeState.languages());
        MainMenuSink menuSink(
            mode,
            menuUi,
            input,
            menuScene,
            stopSignal,
            options,
            optionsScene,
            localeState);

        // The camera it puts back is the one this run opened with.
        // Copied here exactly as the bar's reset button copies it.
        // So the key and the button agree about where back is.
        HotkeySink hotkeySink(options, input, viewCommands);

        // The save screen's own picture, never the menu's or the bar's.
        // Three modes, three overlays, for the reason the menu has one.
        UiOverlay noSaveScreen;
        UiOverlay &saveUi = wiring.saveOverlay.has_value()
                                ? wiring.saveOverlay->get()
                                : noSaveScreen;

        // Restored before the first tick.
        // Through the very store the Load button uses.
        // So the two cannot come out differently.
        if (wiring.start.has_value())
        {
            session.restore(*wiring.start);
        }

        SaveLoadState saveState(wiring.saves);
        const SaveLoadScene saveScene(translator);
        SaveLoadSink saveSink(
            saveState,
            mode,
            saveUi,
            input,
            saveScene,
            session,
            wiring.saveDirectory);

        // Gated on the mode rather than checking one themselves.
        // What a mode changes is what a click means.
        // So engine.tick still reaches both -- see ModeGatedSink.
        ModeGatedSink playingUi(uiSink, mode, AppMode::CityMap);
        ModeGatedSink playingGrid(gridSink, mode, AppMode::CityMap);

        // A hotkey acts on the city, so it is gated like the bar is.
        // The screen the keys are bound on is the main menu's.
        // So choosing a key cannot also fire what it is bound to.
        ModeGatedSink playingHotkeys(hotkeySink, mode, AppMode::CityMap);

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
        // BindingSink is immediately after the mode.
        // Ahead of every sink that reads a key.
        // The machine's layout is announced on the first tick.
        // Before that tick's own input.
        // A sink reading a binding this one has not folded yet.
        // Would be reading a layout nobody was playing.
        // Beside the mode, and for its reason exactly.
        // A language staged last tick lands before anything lays out.
        std::vector<std::reference_wrapper<ITickEventSink>> timedSinks{
            input, mode, localeState, bindingSink, reducer, menuSink,
            saveSink,
            playingHotkeys};

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
        if (wiring.world.has_value())
        {
            timedSinks.push_back(worldSink);
        }

        timedSinks.push_back(playingGrid);
        timedSinks.push_back(stopSignal);

        if (wiring.replayRecorder.has_value())
        {
            timedSinks.push_back(wiring.replayRecorder->get());
        }
        TickedEventDispatcher tickedDispatcher(dispatcher, timedSinks);

        Engine engine(logger, tickedDispatcher);
        Game game(engine, logger);
        game.run();

        EngineLoop loop(engine, tickedDispatcher, wiring.inputSource);
        loop.run(stopSignal, wiring.maxTicks);

        // Taken while the World is still here.
        // A save is read out of it rather than out of the summary.
        antwika::game::saveGameFileIfNamed(
            session.take(), wiring.savePath);

        // The layout this session ended on.
        // Left where the next live run will find it.
        // A replay names nowhere.
        // So replaying somebody else's session rebinds nothing here.
        antwika::game::saveOptionsFileIfNamed(
            PlayerOptions{
                .bindings = options.bindings(),
                .locale = options.locale()},
            wiring.optionsPath);

        const auto frame =
            snapshotOf(world, paths, camera, wiring.extent);

        // Read out here rather than inside the record below.
        // A call among an aggregate's vector members needs a pad.
        // To destroy the half-built record it was made in.
        // Which is a landing pad on a line nothing reaches.
        //
        // Worked out again rather than copied off the local above.
        // That one is what the bar was last told, and is gated.
        // This is what the city amounts to, gate or no gate.
        const auto finalRatings = ratingsOf(world);

        // Every branch left on the excluded line is the allocator's.
        // Two are the throw edges of copying the two vectors.
        // The last is a heap branch nothing here is large enough to take.
        return GameSummary{ // GCOVR_EXCL_LINE
            .state = state,
            .paths = frame.paths,
            .walkers = walkerViewsOf(world),
            .buildings = buildingViewsOf(world),
            .ruins = ruinViewsOf(world),
            .camera = camera,
            .ratings = finalRatings,
            .bindings = options.bindings()};
        // The excluded line is the local summary's unwind destructor.
        // Nothing between its construction and the return throws.
    } // GCOVR_EXCL_LINE

    void printSummary(std::ostream &out, const GameSummary &summary)
    {
        out << "Final state: ticksProcessed="
            << summary.state.ticksProcessed
            << " score=" << summary.state.score
            << " money=" << summary.state.money << '\n';
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
                << building.at.x << ", " << building.at.y << ") "
                << housingLevelName(building.level) << " covered";

            // Every service, including the ones at zero.
            // A summary is read to find out what a run ended up like.
            // "No doctor ever came" is exactly such a fact.
            for (const auto service : kServices)
            {
                out << ' ' << serviceName(service) << '='
                    << building.coverage[serviceIndex(service)];
            }

            out << '\n';
        }

        out << "Ruins: " << summary.ruins.size() << '\n';

        for (const auto &ruin : summary.ruins)
        {
            out << "  " << buildingKindName(ruin.kind) << " at ("
                << ruin.at.x << ", " << ruin.at.y << ") "
                << ruinStateName(ruin.state) << '\n';
        }

        // Every rating, including the ones at zero.
        // A summary is read to find out what a run ended up like.
        // "Nobody ever moved in" is exactly such a fact.
        out << "Ratings: population=" << summary.ratings.population
            << " employment=" << summary.ratings.employment
            << " housing=" << summary.ratings.averageHousingLevel
            << " service=" << summary.ratings.serviceReach << '\n';

        out << "Camera: pan (" << summary.camera.pan().x << ", "
            << summary.camera.pan().y << ") zoom "
            << summary.camera.zoomLevel() << '\n';
    }

} // namespace antwika::game
