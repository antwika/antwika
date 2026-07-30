#include "antwika/game/Game.hpp"

#include <antwika/ecs/SystemScheduler.hpp>
#include <antwika/ecs/World.hpp>
#include <antwika/engine/Engine.hpp>
#include <antwika/engine/StopSignal.hpp>
#include <antwika/event/Event.hpp>
#include <antwika/event/EventDispatcher.hpp>
#include <antwika/event/TickedEventDispatcher.hpp>
#include <antwika/log/Level.hpp>
#include <antwika/replay/EngineLoop.hpp>

#include "antwika/game/BuildingIndex.hpp"
#include "antwika/game/BuildingSystem.hpp"
#include "antwika/game/Events.hpp"
#include "antwika/game/GameStateReducer.hpp"
#include "antwika/game/GridSink.hpp"
#include "antwika/game/InputFold.hpp"
#include "antwika/game/MainMenu.hpp"
#include "antwika/game/MenuSink.hpp"
#include "antwika/game/SceneSnapshot.hpp"
#include "antwika/game/Toolbar.hpp"
#include "antwika/game/UiSink.hpp"
#include "antwika/game/WalkerSystem.hpp"

using antwika::ecs::SystemScheduler;
using antwika::ecs::World;
using antwika::engine::Engine;
using antwika::engine::StopSignal;
using antwika::event::Event;
using antwika::event::EventDispatcher;
using antwika::event::TickedEventDispatcher;
using antwika::log::Level;
using antwika::replay::EngineLoop;

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

        EventDispatcher dispatcher({config.eventSink});

        World world(logger);

        // Which cells are built on, kept beside the world.
        // It is here for the reason PathIndex is.
        // A click has to see what this tick staged and has not committed.
        // Local rather than a config field, since nothing outside reads it.
        // A frame gets its buildings from the world instead.
        BuildingIndex buildings;

        SystemScheduler scheduler;
        WalkerSystem walkerSystem(paths);
        const auto walkPhase = scheduler.createPhase("walk");
        scheduler.addSystem(walkPhase, walkerSystem);

        // A phase after the walk, not beside it.
        // Both of them write the Building component.
        // One write per component per phase survives a commit.
        // So a building drains from what this tick's deliveries left it.
        BuildingSystem buildingSystem(paths, buildings);
        const auto buildPhase = scheduler.createPhase("build");
        scheduler.addSystem(buildPhase, buildingSystem);

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

        // A run nobody handed a menu still gets one.
        // F10 is then a menu whose intents nothing outside reads.
        // Which is what a caller not asking for them means.
        MenuState unreadMenu;
        MenuState &menuState = config.menuState.has_value()
                                   ? config.menuState->get()
                                   : unreadMenu;

        const Toolbar toolbar;
        InputFold input(config.codec);
        UiSink uiSink(camera, ui, input, toolbar, camera);

        // In front of the toolbar's sink rather than beside it.
        // The menu is modal, so while it is up the bar sees nothing.
        // Passing uiSink in is what says so, and UiSink learns nothing.
        const MainMenu mainMenu;
        MenuSink menuSink(menuState, ui, input, mainMenu, uiSink);

        GridSink gridSink(
            world,
            paths,
            buildings,
            camera,
            config.extent,
            scheduler,
            input,
            ui);
        StopSignal stopSignal;

        // The fold is first.
        // What it holds is the event the sinks after it are given now.
        // It is also the only thing that clears an edge.
        // So the tick boundary is one rule in one place.
        // GridSink runs the scheduler on engine.tick.
        // So anything that must show in this frame is folded before it.
        // MenuSink, and the UiSink behind it, still come before it.
        // So a press is resolved against the bar before the grid sees it.
        // And the picture is described before the renderer paints it.
        std::vector<std::reference_wrapper<ITickEventSink>> timedSinks{
            input, reducer};

        // Registered only when there is somewhere to put the picture.
        // Otherwise the bar and menu meet a zero canvas.
        // Which no click can hit, so nothing is ever hovered or pressed.
        // "No toolbar" then means no toolbar, not an unhittable one.
        if (hasToolbar)
        {
            timedSinks.push_back(menuSink);
        }

        timedSinks.push_back(gridSink);
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

        const auto frame =
            snapshotOf(world, paths, camera, config.extent);

        // Every branch left on the excluded line is the allocator's.
        // Two are the throw edges of copying the two vectors.
        // The last is a heap branch nothing here is large enough to take.
        return GameSummary{ // GCOVR_EXCL_LINE
            .state = state,
            .paths = frame.paths,
            .walkers = frame.walkers,
            .buildings = frame.buildings,
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
            out << "  at (" << building.at.x << ", " << building.at.y
                << ") kind "
                << static_cast<int>(building.kind) << " stock "
                << building.held << "/" << building.capacity << '\n';
        }

        out << "Camera: pan (" << summary.camera.pan().x << ", "
            << summary.camera.pan().y << ") zoom "
            << summary.camera.zoomLevel() << '\n';
    }

} // namespace antwika::game
