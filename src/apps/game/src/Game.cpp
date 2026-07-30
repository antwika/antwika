#include "antwika/game/Game.hpp"

#include <antwika/ecs/SystemScheduler.hpp>
#include <antwika/ecs/World.hpp>
#include <antwika/engine/Engine.hpp>
#include <antwika/engine/StopSignal.hpp>
#include <antwika/event/Event.hpp>
#include <antwika/event/EventDispatcher.hpp>
#include <antwika/event/TickedEventDispatcher.hpp>
#include <antwika/replay/EngineLoop.hpp>

#include "antwika/game/Events.hpp"
#include "antwika/game/GameStateReducer.hpp"
#include "antwika/game/GridSink.hpp"
#include "antwika/game/InputFold.hpp"
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
using antwika::replay::EngineLoop;

namespace antwika::game
{

    Game::Game(IEngine &engine,
               IEventDispatcher &dispatcher) : engine(engine),
                                               dispatcher(dispatcher)
    {
    }

    void Game::run()
    {
        dispatcher.dispatch(
            Event{.name = events::kStarted}); // GCOVR_EXCL_LINE
        engine.start();
    }

    GameSummary bootstrap(
        ILogger &logger,
        IEventSink &eventSink,
        IReplaySource &inputSource,
        const IInputEventCodec &codec,
        GridExtent extent,
        Camera &camera,
        PathIndex &paths,
        std::vector<std::reference_wrapper<ISystem>> observers,
        std::optional<antwika::time::Tick> maxTicks,
        ITickEventSink *replayRecorder,
        UiOverlay *overlay)
    {
        EventDispatcher dispatcher({eventSink});

        World world(logger);

        SystemScheduler scheduler;
        WalkerSystem walkerSystem(paths);
        const auto walkPhase = scheduler.createPhase("walk");
        scheduler.addSystem(walkPhase, walkerSystem);

        // A phase of its own.
        // A renderer then sees the generation this walk produced.
        const auto observePhase = scheduler.createPhase("observe");
        for (auto &observer : observers)
        {
            scheduler.addSystem(observePhase, observer.get());
        }

        GameState state;
        GameStateReducer reducer(state);

        // A run with no toolbar still needs something to ask.
        // An overlay nothing writes covers nothing.
        // So every click is the world's, which is what that means.
        UiOverlay noToolbar;
        UiOverlay &ui = overlay != nullptr ? *overlay : noToolbar;

        const Toolbar toolbar;
        InputFold input(codec);
        UiSink uiSink(camera, ui, input, toolbar, camera);
        GridSink gridSink(
            world, paths, camera, extent, scheduler, input, ui);
        StopSignal stopSignal;

        // The fold is first: what it holds is the event the sinks after
        // it are being given now, and it is the only thing that clears
        // an edge, so the tick boundary is one rule in one place.
        // GridSink runs the scheduler on engine.tick.
        // So anything that must show in this frame is folded before it.
        // UiSink still comes before it, so a press is resolved against
        // the bar before the grid sees it, and the picture is described
        // before the renderer paints it.
        std::vector<std::reference_wrapper<ITickEventSink>> timedSinks{
            input, reducer, uiSink, gridSink, stopSignal};
        if (replayRecorder != nullptr)
        {
            timedSinks.push_back(*replayRecorder);
        }
        TickedEventDispatcher tickedDispatcher(dispatcher, timedSinks);

        Engine engine(logger, tickedDispatcher);
        Game game(engine, tickedDispatcher);
        game.run();

        EngineLoop loop(engine, tickedDispatcher, inputSource);
        loop.run(stopSignal, maxTicks);

        const auto frame = snapshotOf(world, paths, camera, extent);

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
