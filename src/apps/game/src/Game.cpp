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
#include <antwika/console/ConsoleGatedSink.hpp>
#include <antwika/console/ConsoleMount.hpp>

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
#include "antwika/game/GameSnapshotStore.hpp"
#include "antwika/game/OptionsConsoleControls.hpp"
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

        LocaleState ownLocale;
        LocaleState &localeState =
            wiring.locale.has_value() ? wiring.locale->get() : ownLocale;
        const Translator &translator = localeState.translator();

        EventDispatcher dispatcher({wiring.eventSink});

        World world(logger);

        AppModeState &mode = wiring.mode;

        SystemScheduler scheduler;
        WalkerSystem walkerSystem(paths, wiring.built, wiring.extent);
        SpawnSystem spawnSystem(paths, wiring.config);
        BuildingSystem buildingSystem(
            wiring.built, wiring.extent, wiring.config);

        SessionGatedSystem gatedWalkers(walkerSystem, mode);

        SessionGatedSystem gatedSpawns(spawnSystem, mode);

        SessionGatedSystem gatedBuildings(buildingSystem, mode);

        PauseState &pause = wiring.pause;
        PauseGatedSystem pausedWalkers(gatedWalkers, pause);
        PauseGatedSystem pausedBuildings(gatedBuildings, pause);
        PauseGatedSystem pausedSpawns(gatedSpawns, pause);

        const auto walkPhase = scheduler.createPhase("walk");
        scheduler.addSystem(walkPhase, pausedWalkers);

        scheduler.addSystem(walkPhase, pausedBuildings);

        scheduler.addSystem(walkPhase, pausedSpawns);

        RuinSystem ruinSystem(
            wiring.built, wiring.extent, wiring.config);

        SessionGatedSystem gatedRuins(ruinSystem, mode);
        PauseGatedSystem pausedRuins(gatedRuins, pause);

        const auto burnPhase = scheduler.createPhase("burn");
        scheduler.addSystem(burnPhase, pausedRuins);

        CoverageSystem coverageSystem;

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

        const auto producePhase = scheduler.createPhase("produce");
        scheduler.addSystem(producePhase, pausedProduction);

        const auto haulPhase = scheduler.createPhase("haul");
        scheduler.addSystem(haulPhase, pausedHauling);

        const auto supplyPhase = scheduler.createPhase("supply");
        scheduler.addSystem(supplyPhase, pausedSupply);

        HousingSystem housingSystem(desirability, wiring.config);

        SessionGatedSystem gatedHousing(housingSystem, mode);
        PauseGatedSystem pausedHousing(gatedHousing, pause);

        const auto settlePhase = scheduler.createPhase("settle");
        scheduler.addSystem(settlePhase, pausedHousing);

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

        const auto observePhase = scheduler.createPhase("observe");

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

        UiOverlay noToolbar;
        const bool hasToolbar = wiring.overlay.has_value();
        UiOverlay &ui = hasToolbar ? wiring.overlay->get() : noToolbar;

        UiOverlay noMenu;
        UiOverlay &menuUi = wiring.menuOverlay.has_value()
                                ? wiring.menuOverlay->get()
                                : noMenu;

        WorldMapState oneCity{WorldMap{}};
        WorldMapState &cities =
            wiring.world.has_value() ? wiring.world->get() : oneCity;

        RoadDrag ownDrag;
        RoadDrag &drag =
            wiring.drag.has_value() ? wiring.drag->get() : ownDrag;

        MapViewState ownView;
        MapViewState &view =
            wiring.view.has_value() ? wiring.view->get() : ownView;

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

        MenuCommands commands(
            mode, session, cities, live, camera, wiring.config);

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

        HotkeySink hotkeySink(options, input, viewCommands);

        UiOverlay noSaveScreen;
        UiOverlay &saveUi = wiring.saveOverlay.has_value()
                                ? wiring.saveOverlay->get()
                                : noSaveScreen;

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
            options,
            wiring.saveDirectory);

        ModeGatedSink playingUi(uiSink, mode, AppMode::CityMap);
        ModeGatedSink playingGrid(gridSink, mode, AppMode::CityMap);

        ModeGatedSink playingHotkeys(hotkeySink, mode, AppMode::CityMap);

        OptionsConsoleControls consoleControls(options);
        GameSnapshotStore snapshotStore(
            session, pause, ui, view, localeState);

        const antwika::console::ConsoleMountSetup consoleSetup{
            .overlay = wiring.consoleOverlay,
            .input = input,
            .store = snapshotStore,
            .dumpPath = wiring.stateDumpPath,
            .loadEnabled = wiring.consoleLoadEnabled,
            .controls = consoleControls,
            .stop = stopSignal}; // GCOVR_EXCL_LINE
        antwika::console::ConsoleMount consoleMount(consoleSetup);

        using antwika::console::ConsoleGatedSink;
        ConsoleGatedSink consoleGatedMenu = consoleMount.gate(menuSink);
        ConsoleGatedSink consoleGatedSave = consoleMount.gate(saveSink);
        ConsoleGatedSink consoleGatedHotkeys =
            consoleMount.gate(playingHotkeys);
        ConsoleGatedSink consoleGatedUi = consoleMount.gate(playingUi);
        ConsoleGatedSink consoleGatedGrid =
            consoleMount.gate(playingGrid);
        ConsoleGatedSink consoleGatedWorld =
            consoleMount.gate(worldSink);

        std::vector<std::reference_wrapper<ITickEventSink>> timedSinks{
            input, mode, localeState, bindingSink, reducer};

        if (consoleMount.mounted())
        {
            timedSinks.push_back(consoleMount.sink());
        }

        timedSinks.push_back(consoleGatedMenu);
        timedSinks.push_back(consoleGatedSave);
        timedSinks.push_back(consoleGatedHotkeys);

        if (hasToolbar)
        {
            timedSinks.push_back(consoleGatedUi);
        }

        if (wiring.world.has_value())
        {
            timedSinks.push_back(consoleGatedWorld);
        }

        timedSinks.push_back(consoleGatedGrid);
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

        antwika::game::saveGameFileIfNamed(
            session.take(), wiring.savePath);

        antwika::game::saveOptionsFileIfNamed(
            PlayerOptions{
                .bindings = options.bindings(),
                .locale = options.locale(),
                .keyboard = options.keyboard()},
            wiring.optionsPath);

        const auto frame =
            snapshotOf(world, paths, camera, wiring.extent);

        const auto finalRatings = ratingsOf(world);

        return GameSummary{ // GCOVR_EXCL_LINE
            .state = state,
            .paths = frame.paths,
            .walkers = walkerViewsOf(world),
            .buildings = buildingViewsOf(world),
            .ruins = ruinViewsOf(world),
            .camera = camera,
            .ratings = finalRatings,
            .console = consoleMount.state().history(),
            .keyboard = options.keyboard(),
            .bindings = options.bindings()};
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

        out << "Ratings: population=" << summary.ratings.population
            << " employment=" << summary.ratings.employment
            << " housing=" << summary.ratings.averageHousingLevel
            << " service=" << summary.ratings.serviceReach << '\n';

        out << "Console lines: " << summary.console.size() << '\n';

        out << "Camera: pan (" << summary.camera.pan().x << ", "
            << summary.camera.pan().y << ") zoom "
            << summary.camera.zoomLevel() << '\n';
    }

}
