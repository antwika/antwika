#include "antwika/life/Life.hpp"

#include <memory>

#include <antwika/ecs/SystemScheduler.hpp>
#include <antwika/ecs/World.hpp>
#include <antwika/engine/Engine.hpp>
#include <antwika/engine/StopSignal.hpp>
#include <antwika/event/Event.hpp>
#include <antwika/event/EventDispatcher.hpp>
#include <antwika/event/TickedEventDispatcher.hpp>
#include <antwika/log/Level.hpp>
#include <antwika/replay/EngineLoop.hpp>

#include "antwika/life/BoardSink.hpp"
#include "antwika/life/DragPausedSystem.hpp"
#include "antwika/life/Events.hpp"
#include "antwika/life/Grid.hpp"
#include "antwika/life/LifeSystem.hpp"

using antwika::ecs::SystemScheduler;
using antwika::ecs::World;
using antwika::engine::Engine;
using antwika::engine::StopSignal;
using antwika::event::Event;
using antwika::event::EventDispatcher;
using antwika::event::TickedEventDispatcher;
using antwika::log::Level;
using antwika::replay::EngineLoop;

namespace antwika::life
{

    Life::Life(IEngine &engine, ILogger &logger)
        : engine(engine), logger(logger)
    {
    }

    void Life::run()
    {
        logger.log(Level::Info, "Running Antwika Life");
        engine.start();
    }

    Board bootstrap(
        ILogger &logger,
        IEventSink &eventSink,
        IReplaySource &inputSource,
        std::uint32_t width,
        std::uint32_t height,
        std::vector<std::reference_wrapper<ISystem>> observers,
        std::optional<antwika::time::Tick> maxTicks,
        ITickEventSink *replayRecorder,
        const TickSinkFactory &extraSink)
    {
        EventDispatcher dispatcher({eventSink});

        World world(logger);
        Grid grid(world, width, height);
        world.commit();

        SystemScheduler scheduler;
        LifeSystem lifeSystem(grid);

        // A board being drawn on stands still.
        // A cell toggled on one tick is then still there on the next.
        // Only a sink reporting a drag can ever start one.
        // A run that registered none is therefore unaffected.
        DragState drag;
        DragPausedSystem pausedLife(lifeSystem, drag);

        const auto lifePhase = scheduler.createPhase("life");
        scheduler.addSystem(lifePhase, pausedLife);

        const auto observePhase = scheduler.createPhase("observe");
        for (auto &observer : observers)
        {
            scheduler.addSystem(observePhase, observer.get());
        }

        BoardSink boardSink(world, grid, scheduler);
        StopSignal stopSignal;

        std::vector<std::reference_wrapper<ITickEventSink>> timedSinks{
            boardSink, stopSignal};

        // Held out here rather than inside the if.
        // The sink has to outlive the reference the dispatcher keeps.
        std::unique_ptr<ITickEventSink> extra;
        if (extraSink)
        {
            extra = extraSink(world, grid, drag);
            timedSinks.push_back(*extra);
        }

        if (replayRecorder != nullptr)
        {
            timedSinks.push_back(*replayRecorder);
        }
        TickedEventDispatcher tickedDispatcher(dispatcher, timedSinks);

        Engine engine(logger, tickedDispatcher);
        Life life(engine, logger);
        life.run();

        EngineLoop loop(engine, tickedDispatcher, inputSource);
        loop.run(stopSignal, maxTicks);

        return readBoard(world, grid);
    }

} // namespace antwika::life
