#include "antwika/life/Life.hpp"

#include <antwika/ecs/SystemScheduler.hpp>
#include <antwika/ecs/World.hpp>
#include <antwika/engine/Engine.hpp>
#include <antwika/engine/StopSignal.hpp>
#include <antwika/event/Event.hpp>
#include <antwika/event/EventDispatcher.hpp>
#include <antwika/event/TickedEventDispatcher.hpp>
#include <antwika/log/Logger.hpp>
#include <antwika/replay/EngineLoop.hpp>

#include "antwika/life/BoardSink.hpp"
#include "antwika/life/Grid.hpp"
#include "antwika/life/LifeSystem.hpp"

using antwika::ecs::SystemScheduler;
using antwika::ecs::World;
using antwika::engine::Engine;
using antwika::engine::StopSignal;
using antwika::event::Event;
using antwika::event::EventDispatcher;
using antwika::event::TickedEventDispatcher;
using antwika::log::Logger;
using antwika::replay::EngineLoop;

namespace antwika::life
{

    Life::Life(IEngine &engine, IEventDispatcher &dispatcher)
        : engine(engine), dispatcher(dispatcher)
    {
    }

    void Life::run()
    {
        dispatcher.dispatch(
            Event{.name = "Running Antwika Life"}); // GCOVR_EXCL_LINE
        engine.start();
    }

    Board bootstrap(
        IClock &clock,
        IAppender &appender,
        IFormatter &formatter,
        ILogPolicy &logPolicy,
        IEventSink &eventSink,
        IReplaySource &inputSource,
        std::uint32_t width,
        std::uint32_t height,
        std::vector<std::reference_wrapper<ISystem>> observers,
        std::optional<antwika::time::Tick> maxTicks,
        ITickEventSink *replayRecorder)
    {
        Logger logger(formatter, logPolicy, clock, appender);
        EventDispatcher dispatcher({eventSink});

        World world(logger);
        Grid grid(world, width, height);
        world.commit();

        SystemScheduler scheduler;
        LifeSystem lifeSystem(grid);
        const auto lifePhase = scheduler.createPhase("life");
        scheduler.addSystem(lifePhase, lifeSystem);

        const auto observePhase = scheduler.createPhase("observe");
        for (auto &observer : observers)
        {
            scheduler.addSystem(observePhase, observer.get());
        }

        BoardSink boardSink(world, grid, scheduler);
        StopSignal stopSignal;

        std::vector<std::reference_wrapper<ITickEventSink>> timedSinks{
            boardSink, stopSignal};
        if (replayRecorder != nullptr)
        {
            timedSinks.push_back(*replayRecorder);
        }
        TickedEventDispatcher tickedDispatcher(dispatcher, timedSinks);

        Engine engine(logger, tickedDispatcher);
        Life life(engine, tickedDispatcher);
        life.run();

        EngineLoop loop(engine, tickedDispatcher, inputSource);
        loop.run(stopSignal, maxTicks);

        return readBoard(world, grid);
    }

} // namespace antwika::life
