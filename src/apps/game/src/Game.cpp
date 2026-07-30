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

        SystemScheduler scheduler;
        WalkerSystem walkerSystem(paths);
        const auto walkPhase = scheduler.createPhase("walk");
        scheduler.addSystem(walkPhase, walkerSystem);

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

        const Toolbar toolbar;
        UiSink uiSink(camera, ui, config.codec, toolbar, camera);
        GridSink gridSink(
            world,
            paths,
            camera,
            config.extent,
            scheduler,
            config.codec,
            ui);
        StopSignal stopSignal;

        // GridSink runs the scheduler on engine.tick.
        // So anything that must show in this frame is folded before it.
        // UiSink comes before it for both reasons.
        // A press is resolved against the bar before the grid sees it.
        // And the picture is described before the renderer paints it.
        std::vector<std::reference_wrapper<ITickEventSink>> timedSinks{
            reducer};

        // Registered only when there is somewhere to put the picture.
        // Otherwise the bar is described against a zero canvas.
        // Which no click can hit, so nothing is ever hovered or pressed.
        // "No toolbar" then means no toolbar, not an unhittable one.
        if (hasToolbar)
        {
            timedSinks.push_back(uiSink);
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
            .camera = camera};
        // The excluded line is the local summary's unwind destructor.
        // Nothing between its construction and the return throws.
    } // GCOVR_EXCL_LINE

} // namespace antwika::game
