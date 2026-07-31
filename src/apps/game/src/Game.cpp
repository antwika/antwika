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

#include "antwika/game/Events.hpp"
#include "antwika/game/GameStateReducer.hpp"
#include "antwika/game/GridSink.hpp"
#include "antwika/game/InputFold.hpp"
#include "antwika/game/MainMenuScene.hpp"
#include "antwika/game/MainMenuSink.hpp"
#include "antwika/game/ModeGatedSink.hpp"
#include "antwika/game/ModeGatedSystem.hpp"
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

        AppModeState &mode = config.mode;

        SystemScheduler scheduler;
        WalkerSystem walkerSystem(paths);

        // The walkers stop with the grid they walk on.
        // Only that one system stops.
        // The tick, the commit and every observer still run.
        // So the menu is drawn and the run is still paced.
        ModeGatedSystem gatedWalkers(
            walkerSystem, mode, AppMode::Playing);
        const auto walkPhase = scheduler.createPhase("walk");
        scheduler.addSystem(walkPhase, gatedWalkers);

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

        const Toolbar toolbar;
        InputFold input(config.codec);
        UiSink uiSink(camera, ui, input, toolbar, camera);
        GridSink gridSink(
            world,
            paths,
            camera,
            config.extent,
            scheduler,
            input,
            ui);
        StopSignal stopSignal;

        const MainMenuScene menuScene;
        MainMenuSink menuSink(
            mode, menuUi, input, menuScene, stopSignal);

        // Gated on the mode rather than checking one themselves.
        // What a mode changes is what a click means.
        // So engine.tick still reaches both -- see ModeGatedSink.
        ModeGatedSink playingUi(uiSink, mode, AppMode::Playing);
        ModeGatedSink playingGrid(gridSink, mode, AppMode::Playing);

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
        std::vector<std::reference_wrapper<ITickEventSink>> timedSinks{
            input, mode, reducer, menuSink};

        // Registered only when there is somewhere to put the picture.
        // Otherwise the bar is described against a zero canvas.
        // Which no click can hit, so nothing is ever hovered or pressed.
        // "No toolbar" then means no toolbar, not an unhittable one.
        if (hasToolbar)
        {
            timedSinks.push_back(playingUi);
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

        const auto frame =
            snapshotOf(world, paths, camera, config.extent);

        // Every branch left on the excluded line is the allocator's.
        // Two are the throw edges of copying the two vectors.
        // The last is a heap branch nothing here is large enough to take.
        return GameSummary{ // GCOVR_EXCL_LINE
            .state = state,
            .paths = frame.paths,
            .walkers = frame.walkers,
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

        out << "Camera: pan (" << summary.camera.pan().x << ", "
            << summary.camera.pan().y << ") zoom "
            << summary.camera.zoomLevel() << '\n';
    }

} // namespace antwika::game
